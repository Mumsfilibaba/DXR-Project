#include "Application/Elements/VisualElement.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/DrawCache.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"

static TAutoConsoleVariable<bool> CVarDrawCacheEnable(
    "UI.DrawCache.Enable",
    "Replays the draw commands a clean subtree recorded before instead of walking it again",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarDrawCacheMinCommands(
    "UI.DrawCache.MinCommands",
    "How many commands a subtree has to emit before keeping its recording is worth the copy",
    24,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarDrawCacheMinCleanFrames(
    "UI.DrawCache.MinCleanFrames",
    "How many frames in a row a subtree has to stay clean before its recording is kept",
    2,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarDrawCacheMaxDirtyFrames(
    "UI.DrawCache.MaxDirtyFrames",
    "How many recent frames may dirty a subtree before its recording stops being cached at all",
    3,
    EConsoleVariableFlags::Default);

FVisualElement::FVisualElement()
    : TSharedFromThis<FVisualElement>()
    , Visibility(EVisibility::Visible)
    , ActivationPolicy(EElementActivationPolicy::DoNotAutoFocusOnWindowActivate)
    , ContentRectangle()
    , CachedDesiredSize()
    , ParentElement()
    , DrawCacheBlock(nullptr)
    , DrawCachePolicy(EDrawCachePolicy::Auto)
    , LastRecordedCommandCount(0)
    , bDrawCacheBlocked(false)
    , CleanPaintFrameCount(0)
    , RecentDirtyFrameCount(0)
    , bDesiredSizeDirty(true)
    , bPaintDirty(true)
{
}

FVisualElement::~FVisualElement()
{
}

void FVisualElement::Tick(const FRectangle& AssignedBounds)
{
    SetContentRectangle(AssignedBounds);
    OnArrange(AssignedBounds);
}

bool FVisualElement::IsWindow() const
{
    return false;
}

bool FVisualElement::IsInteractive() const
{
    return false;
}

bool FVisualElement::CapturesAllInput() const
{
    return false;
}

bool FVisualElement::SupportsKeyboardFocus() const
{
    return false;
}

bool FVisualElement::WantsTextInput() const
{
    return false;
}

TSharedPtr<FVisualElement> FVisualElement::GetFocusTarget()
{
    return AsSharedPtr();
}

FEventResponse FVisualElement::OnAnalogGamepadChange(const FAnalogGamepadEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyDown(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyUp(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyChar(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseMove(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseButtonDown(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseButtonUp(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseScroll(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseDoubleClick(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseLeft(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseEntered(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnHighPrecisionMouseInput(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

bool FVisualElement::GetCursor(ECursor&) const
{
    return false;
}

FEventResponse FVisualElement::OnFocusLost()
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnFocusGained()
{
    return FEventResponse::Unhandled();
}

IntVector2 FVisualElement::ComputeDesiredSize() const
{
    return IntVector2(0, 0);
}

void FVisualElement::OnArrange(const FRectangle& /*AllottedBounds*/)
{
}

void FVisualElement::GetChildren(TArray<TSharedPtr<FVisualElement>>& /*OutChildren*/) const
{
}

int32 FVisualElement::OnDraw(const FDrawGeometry& /*AllottedGeometry*/, FDrawCommandList& /*OutCommandList*/, int32 LayerId) const
{
    return LayerId;
}

int32 FVisualElement::Draw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (DrawCachePolicy == EDrawCachePolicy::Never)
    {
        OutCommandList.BlockDrawCache();

        bPaintDirty = false;
        return OnDraw(AllottedGeometry, OutCommandList, LayerId);
    }

    if (!CVarDrawCacheEnable.GetValue() || OutCommandList.IsDrawCacheSuppressed())
    {
        bPaintDirty = false;
        return OnDraw(AllottedGeometry, OutCommandList, LayerId);
    }

    if (!bPaintDirty && DrawCacheBlock)
    {
        const int32       ClipDepth     = OutCommandList.GetClipDepth();
        const FRectangle& ClipRectangle = OutCommandList.GetCurrentClipRectangle();

        if (DrawCacheBlock->CanReplay(AllottedGeometry, LayerId, ClipDepth, ClipRectangle))
        {
            DrawCacheBlock->LastUsedFrame = DrawCacheRegistry::GetCurrentFrame();

            CleanPaintFrameCount = TNumericLimits<uint8>::Max();
            return OutCommandList.AppendDrawCache(*DrawCacheBlock);
        }
    }

    const int32 CommandBase = OutCommandList.GetCommands().Size();

    if (OutCommandList.IsDrawCacheOpen() || !ShouldUseDrawCache())
    {
        DrawCacheBlock.Reset();

        bPaintDirty = false;
        const int32 MaxLayerId = OnDraw(AllottedGeometry, OutCommandList, LayerId);

        NoteWalked(OutCommandList.GetCommands().Size() - CommandBase);
        return MaxLayerId;
    }

    const FDrawCommandList::FDrawCacheMarker Marker = OutCommandList.BeginDrawCache();

    bPaintDirty = false;
    const int32 MaxLayerId = OnDraw(AllottedGeometry, OutCommandList, LayerId);

    const bool  bStillClean  = !bPaintDirty;
    const int32 CommandCount = OutCommandList.GetCommands().Size() - CommandBase;

    const bool bWorthKeeping = bStillClean
        && (DrawCachePolicy == EDrawCachePolicy::Always || CommandCount >= CVarDrawCacheMinCommands.GetValue());

    if (!bWorthKeeping)
    {
        OutCommandList.AbandonDrawCache();
        DrawCacheBlock.Reset();
    }
    else
    {
        if (!DrawCacheBlock)
        {
            DrawCacheBlock = MakeUniquePtr<FDrawCacheBlock>();
        }

        if (!OutCommandList.CaptureDrawCache(Marker, AllottedGeometry, LayerId, MaxLayerId, *DrawCacheBlock))
        {
            DrawCacheBlock->Reset();
        }
    }

    if (OutCommandList.GetDrawCacheBlockCounter() != Marker.BlockCounter)
    {
        bDrawCacheBlocked = true;
    }

    NoteWalked(CommandCount);
    return MaxLayerId;
}

bool FVisualElement::ShouldUseDrawCache() const
{
    if (DrawCachePolicy == EDrawCachePolicy::Always)
    {
        return true;
    }

    if (bDrawCacheBlocked)
    {
        return false;
    }

    if (RecentDirtyFrameCount > static_cast<uint8>(Math::Clamp(CVarDrawCacheMaxDirtyFrames.GetValue(), 0, 255)))
    {
        return false;
    }

    if (CleanPaintFrameCount < static_cast<uint8>(Math::Clamp(CVarDrawCacheMinCleanFrames.GetValue(), 0, 255)))
    {
        return false;
    }

    return LastRecordedCommandCount == 0
        || LastRecordedCommandCount >= static_cast<uint16>(Math::Clamp(CVarDrawCacheMinCommands.GetValue(), 0, 65535));
}

void FVisualElement::NoteWalked(int32 CommandCount) const
{
    LastRecordedCommandCount = static_cast<uint16>(Math::Clamp(CommandCount, 0, 65535));

    if (CleanPaintFrameCount < TNumericLimits<uint8>::Max())
    {
        ++CleanPaintFrameCount;
    }

    if (RecentDirtyFrameCount > 0 && CleanPaintFrameCount > RecentDirtyFrameCount)
    {
        --RecentDirtyFrameCount;
    }
}

void FVisualElement::SetOuterCornerRadius(float /*InCornerRadius*/)
{
}

int32 FVisualElement::GetContentTopInset() const
{
    return 0;
}

IntVector2 FVisualElement::PrepareDesiredSize()
{
    if (!bDesiredSizeDirty)
    {
        return CachedDesiredSize;
    }

    TArray<TSharedPtr<FVisualElement>> Children;
    GetChildren(Children);

    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        if (Child)
        {
            Child->PrepareDesiredSize();
        }
    }

    bDesiredSizeDirty = false;

    const IntVector2 DesiredSize = ComputeDesiredSize();
    if (DesiredSize != CachedDesiredSize)
    {
        CachedDesiredSize = DesiredSize;

        if (TSharedPtr<FVisualElement> Parent = ParentElement.ToSharedPtr())
        {
            Parent->InvalidateDesiredSize();
        }
    }

    return CachedDesiredSize;
}

void FVisualElement::InvalidateDesiredSize()
{
    InvalidatePaint();

    bDrawCacheBlocked = false;

    if (bDesiredSizeDirty)
    {
        return;
    }

    bDesiredSizeDirty = true;

    if (TSharedPtr<FVisualElement> Parent = ParentElement.ToSharedPtr())
    {
        Parent->InvalidateDesiredSize();
    }
}

void FVisualElement::InvalidatePaint()
{
    if (bPaintDirty)
    {
        return;
    }

    bPaintDirty          = true;
    CleanPaintFrameCount = 0;

    if (RecentDirtyFrameCount < TNumericLimits<uint8>::Max())
    {
        ++RecentDirtyFrameCount;
    }

    if (TSharedPtr<FVisualElement> Parent = ParentElement.ToSharedPtr())
    {
        Parent->InvalidatePaint();
    }
}

void FVisualElement::RequestContinuousPaint() const
{
    bPaintDirty          = true;
    CleanPaintFrameCount = 0;

    for (FVisualElement* Parent = GetParentElement().Get(); Parent; Parent = Parent->GetParentElement().Get())
    {
        Parent->bPaintDirty          = true;
        Parent->CleanPaintFrameCount = 0;
    }
}

void FVisualElement::SetDrawCachePolicy(EDrawCachePolicy InPolicy)
{
    if (DrawCachePolicy == InPolicy)
    {
        return;
    }

    DrawCachePolicy = InPolicy;

    if (DrawCachePolicy == EDrawCachePolicy::Never)
    {
        ReleaseDrawCache();
    }

    InvalidatePaint();
}

void FVisualElement::ReleaseDrawCache()
{
    DrawCacheBlock.Reset();
}

bool FVisualElement::HasDrawCache() const
{
    return DrawCacheBlock && DrawCacheBlock->bValid;
}

void FVisualElement::FindParentElements(FElementPath& OutRootPath)
{
    if (ParentElement.IsValid())
    {
        ParentElement->FindParentElements(OutRootPath);
    }

    OutRootPath.Add(Visibility, AsSharedPtr());
}

void FVisualElement::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    if (ContentRectangle.EncapsulatesPoint(ClientPosition))
    {
        OutChildElements.Add(Visibility, AsSharedPtr());
    }
}

void FVisualElement::SetVisibility(EVisibility InVisibility)
{
    if (Visibility != InVisibility)
    {
        Visibility = InVisibility;
        InvalidateDesiredSize();
    }
}

void FVisualElement::SetParentElement(const TWeakPtr<FVisualElement>& InParentElement)
{
    TSharedPtr<FVisualElement> PreviousParent = ParentElement.ToSharedPtr();
    ParentElement = InParentElement;

    InvalidateDesiredSize();

    TSharedPtr<FVisualElement> Parent = ParentElement.ToSharedPtr();
    if (Parent)
    {
        Parent->InvalidateDesiredSize();
    }

    if (PreviousParent && PreviousParent != Parent)
    {
        PreviousParent->InvalidateDesiredSize();
    }
}

void FVisualElement::SetContentRectangle(const FRectangle& InContentRectangle)
{
    if (ContentRectangle == InContentRectangle)
    {
        return;
    }

    ContentRectangle = InContentRectangle;
    InvalidatePaint();
}
