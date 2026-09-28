#include "Application/Draw/DrawCache.h"
#include "Application/Text/FontAtlas.h"
#include "Core/Algorithms/Algorithm.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"

static TAutoConsoleVariable<int32> CVarDrawCacheBudgetKB(
    "UI.DrawCache.BudgetKB",
    "How much memory every draw cache block may hold between them, in kilobytes",
    8192,
    EConsoleVariableFlags::Default);

static FAutoConsoleCommand CCmdDumpDrawCacheStats(
    "UI.DumpDrawCacheStats",
    "Logs how many subtrees are keeping their recording and how much memory those recordings hold",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        DrawCacheRegistry::DumpStats();
    }));

static TArray<FDrawCacheBlock*> GDrawCacheBlocks;
static int64                    GDrawCacheByteSize     = 0;
static int64                    GDrawCachePeakByteSize = 0;
static uint64                   GDrawCacheFrameNumber  = 0;

static uint64 GDrawCacheEpoch = 1;

template<typename ElementType>
static FORCEINLINE int64 ArrayByteSize(const TArray<ElementType>& Array)
{
    return static_cast<int64>(Array.Capacity()) * static_cast<int64>(sizeof(ElementType));
}

void DrawCacheEpoch::Advance()
{
    ++GDrawCacheEpoch;

    DrawCacheRegistry::ReleaseAll();
}

uint64 DrawCacheEpoch::Get()
{
    return GDrawCacheEpoch;
}

FDrawCacheBlock::FDrawCacheBlock()
    : Commands()
    , Points()
    , TextPool()
    , Brushes()
    , ClipRects()
    , Vertices()
    , Indices()
    , ShapeInstances()
    , TextGlyphInstances()
    , Batches()
    , AtlasDependencies()
    , CapturedRectangle()
    , CapturedClipRectangle()
    , CapturedEpoch(0)
    , CapturedScale(1.0f)
    , BaseLayerId(0)
    , MaxLayerId(0)
    , CapturedClipDepth(0)
    , TextCommandCount(0)
    , LastUsedFrame(0)
    , bValid(false)
    , bGeometryValid(false)
    , bGeometryContiguous(true)
    , bHasCapturedClip(false)
{
    DrawCacheRegistry::Register(this);
}

FDrawCacheBlock::~FDrawCacheBlock()
{
    DrawCacheRegistry::Unregister(this);
}

void FDrawCacheBlock::Reset()
{
    const int64 PreviousByteSize = GetByteSize();

    Commands.Clear();
    Points.Clear();
    TextPool.Clear();
    Brushes.Clear();
    ClipRects.Clear();

    Vertices.Clear();
    Indices.Clear();
    ShapeInstances.Clear();
    TextGlyphInstances.Clear();
    Batches.Clear();

    AtlasDependencies.Clear();

    CapturedRectangle     = FRectangle();
    CapturedClipRectangle = FRectangle();
    CapturedEpoch         = 0;
    CapturedScale         = 1.0f;
    BaseLayerId           = 0;
    MaxLayerId            = 0;
    CapturedClipDepth     = 0;
    TextCommandCount      = 0;
    bValid                = false;
    bGeometryValid        = false;
    bGeometryContiguous   = true;
    bHasCapturedClip      = false;

    DrawCacheRegistry::NotifySizeChanged(GetByteSize() - PreviousByteSize);
}

int64 FDrawCacheBlock::GetByteSize() const
{
    return ArrayByteSize(Commands)
        + ArrayByteSize(Points)
        + ArrayByteSize(TextPool)
        + ArrayByteSize(Brushes)
        + ArrayByteSize(ClipRects)
        + ArrayByteSize(Vertices)
        + ArrayByteSize(Indices)
        + ArrayByteSize(ShapeInstances)
        + ArrayByteSize(TextGlyphInstances)
        + ArrayByteSize(Batches)
        + ArrayByteSize(AtlasDependencies);
}

bool FDrawCacheBlock::CanReplay(const FDrawGeometry& AllottedGeometry, int32 InBaseLayerId, int32 ClipDepth, const FRectangle& ClipRectangle) const
{
    if (!bValid || CapturedEpoch != DrawCacheEpoch::Get())
    {
        return false;
    }

    if (CapturedRectangle != AllottedGeometry.Bounds || CapturedScale != AllottedGeometry.Scale)
    {
        return false;
    }

    if (BaseLayerId != InBaseLayerId || CapturedClipDepth != ClipDepth)
    {
        return false;
    }

    if (bHasCapturedClip && CapturedClipRectangle != ClipRectangle)
    {
        return false;
    }

    return AreAtlasDependenciesCurrent();
}

bool FDrawCacheBlock::AreAtlasDependenciesCurrent() const
{
    for (const FDrawCacheAtlasDependency& Dependency : AtlasDependencies)
    {
        if (!Dependency.Atlas || Dependency.Atlas->GetRevision() != Dependency.Revision)
        {
            return false;
        }
    }

    return true;
}

void DrawCacheRegistry::Register(FDrawCacheBlock* Block)
{
    if (!Block)
    {
        return;
    }

    GDrawCacheBlocks.Add(Block);
}

void DrawCacheRegistry::Unregister(FDrawCacheBlock* Block)
{
    if (!Block)
    {
        return;
    }

    const int32 Index = GDrawCacheBlocks.Find(Block);
    if (Index >= 0)
    {
        GDrawCacheByteSize -= Block->GetByteSize();
        GDrawCacheBlocks.RemoveAtSwap(Index);
    }

    if (GDrawCacheByteSize < 0)
    {
        GDrawCacheByteSize = 0;
    }
}

void DrawCacheRegistry::NotifySizeChanged(int64 ByteDelta)
{
    GDrawCacheByteSize += ByteDelta;
    if (GDrawCacheByteSize < 0)
    {
        GDrawCacheByteSize = 0;
    }
}

void DrawCacheRegistry::EnforceBudget()
{
    const int64 BudgetBytes = static_cast<int64>(Math::Max(CVarDrawCacheBudgetKB.GetValue(), 0)) * 1024ll;
    if (BudgetBytes <= 0 || GDrawCacheByteSize <= BudgetBytes)
    {
        return;
    }

    TArray<FDrawCacheBlock*> Candidates;
    Candidates.Reserve(GDrawCacheBlocks.Size());

    for (FDrawCacheBlock* Block : GDrawCacheBlocks)
    {
        if (Block && Block->bValid)
        {
            Candidates.Add(Block);
        }
    }

    Algorithm::Sort(Candidates, [](const FDrawCacheBlock* Left, const FDrawCacheBlock* Right)
    {
        return Left->LastUsedFrame < Right->LastUsedFrame;
    });

    for (FDrawCacheBlock* Block : Candidates)
    {
        if (GDrawCacheByteSize <= BudgetBytes)
        {
            break;
        }

        Block->Reset();
    }
}

void DrawCacheRegistry::ReleaseAll()
{
    for (FDrawCacheBlock* Block : GDrawCacheBlocks)
    {
        if (Block)
        {
            Block->Reset();
        }
    }
}

int64 DrawCacheRegistry::GetTotalByteSize()
{
    return GDrawCacheByteSize;
}

int32 DrawCacheRegistry::GetBlockCount()
{
    return GDrawCacheBlocks.Size();
}

int64 DrawCacheRegistry::GetPeakByteSize()
{
    return GDrawCachePeakByteSize;
}

void DrawCacheRegistry::DumpStats()
{
    int32 ValidBlocks   = 0;
    int32 TotalCommands = 0;

    for (const FDrawCacheBlock* Block : GDrawCacheBlocks)
    {
        if (Block && Block->bValid)
        {
            ++ValidBlocks;
            TotalCommands += Block->GetCommandCount();
        }
    }

    const int64 BudgetBytes = static_cast<int64>(Math::Max(CVarDrawCacheBudgetKB.GetValue(), 0)) * 1024ll;

    LOG_INFO("[DrawCache] %d of %d blocks hold a recording, replaying %d commands between them",
        ValidBlocks, GDrawCacheBlocks.Size(), TotalCommands);

    LOG_INFO("[DrawCache] holding %lld bytes, peak %lld, budget %lld (%.1f%% used)",
        GDrawCacheByteSize, GDrawCachePeakByteSize, BudgetBytes,
        BudgetBytes > 0 ? (100.0 * static_cast<double>(GDrawCacheByteSize) / static_cast<double>(BudgetBytes)) : 0.0);

    if (ValidBlocks > 0)
    {
        LOG_INFO("[DrawCache] averaging %lld bytes per block and %d commands per block",
            GDrawCacheByteSize / ValidBlocks, TotalCommands / ValidBlocks);
    }
}

void DrawCacheRegistry::BeginFrame()
{
    ++GDrawCacheFrameNumber;
    EnforceBudget();

    GDrawCachePeakByteSize = Math::Max(GDrawCachePeakByteSize, GDrawCacheByteSize);
}

uint64 DrawCacheRegistry::GetCurrentFrame()
{
    return GDrawCacheFrameNumber;
}
