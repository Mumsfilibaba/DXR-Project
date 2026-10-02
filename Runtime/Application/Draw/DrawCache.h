#pragma once
#include "Core/Containers/Array.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Draw/UIDrawData.h"

class FFontAtlas;

struct APPLICATION_API DrawCacheEpoch
{
    /** @brief Moves the epoch on, which invalidates every block recorded before this call. */
    static void Advance();

    /** @return The value a block has to match to still be replayable. */
    NODISCARD static uint64 Get();
};

struct FDrawCacheAtlasDependency
{
    FDrawCacheAtlasDependency()
        : Atlas(nullptr)
        , Revision(0)
    {
    }

    FDrawCacheAtlasDependency(const FFontAtlas* InAtlas, uint64 InRevision)
        : Atlas(InAtlas)
        , Revision(InRevision)
    {
    }

    const FFontAtlas* Atlas;
    uint64            Revision;
};

struct APPLICATION_API FDrawCacheBlock : public FNonCopyable
{
    FDrawCacheBlock();
    ~FDrawCacheBlock();

    /** @brief Drops everything the block holds and marks it unusable, keeping the allocations for reuse. */
    void Reset();

    /** @return Roughly how much memory the block holds, which is what the budget is spent from. */
    NODISCARD int64 GetByteSize() const;

    /** @return True when every atlas the block drew glyphs from is still at the revision it captured. */
    NODISCARD bool AreAtlasDependenciesCurrent() const;

    /**
     * @brief Whether the block can be spliced into a list standing where it was recorded, or moved without resizing
     * together with the clip it was recorded under.
     *
     * @param AllottedGeometry The geometry the element is being drawn into now.
     * @param InBaseLayerId    The layer the element is being asked to draw on now.
     * @param ClipDepth        How many clips are open around the element now.
     * @param ClipRectangle    The clip the element would be cut against now.
     * @param OutOffset        How far the element moved since it recorded, which Translate has to apply first.
     */
    NODISCARD bool CanReplay(
        const FDrawGeometry& AllottedGeometry,
        int32                InBaseLayerId,
        int32                ClipDepth,
        const FRectangle&    ClipRectangle,
        IntVector2&          OutOffset) const;

    /**
     * @brief Moves everything the block recorded, so it replays where the element stands now. The captured geometry
     * was built at the old position, so it is dropped and captured again on the next splice.
     *
     * @param Offset How far to move, in pixels.
     */
    void Translate(const IntVector2& Offset);

    /** @return The number of commands the block replays, which is also its span in the frame's command list. */
    NODISCARD FORCEINLINE int32 GetCommandCount() const
    {
        return Commands.Size();
    }

    /** @brief The recorded commands, with every payload offset and clip id rebased to index this block. */
    TArray<FDrawCommand> Commands;

    /** @brief The points the polyline and polygon commands index. */
    TArray<Vector2> Points;

    /** @brief The characters the text commands index, packed end to end without terminators. */
    TArray<CHAR> TextPool;

    /** @brief The brushes the image commands index, each holding a raw texture pointer the epoch guards. */
    TArray<FUIBrush> Brushes;

    /** @brief The rectangles the commands' clip ids index, cut against the clip open as the block was entered. */
    TArray<FRectangle> ClipRects;

    /** @brief The vertices the commands tessellated into, valid only while bGeometryValid. */
    TArray<FUIVertex> Vertices;

    /** @brief Indices into Vertices, counted from the block's own first vertex so they survive a different base. */
    TArray<uint32> Indices;

    /** @brief The instanced shapes the commands tessellated into, valid only while bGeometryValid. */
    TArray<FUIShapeInstance> ShapeInstances;

    /** @brief The instanced glyphs the text commands tessellated into, valid only while bGeometryValid. */
    TArray<FUITextGlyphInstance> TextGlyphInstances;

    /** @brief How the geometry above divides into draw calls, the first of which can merge into one already open. */
    TArray<FUIDrawBatch> Batches;

    /** @brief Every atlas the block's glyphs were packed from, so a repack can be noticed. */
    TArray<FDrawCacheAtlasDependency> AtlasDependencies;

    /** @brief The arranged rectangle the element had when it recorded, which it has to still have. */
    FRectangle CapturedRectangle;

    /** @brief The clip the enclosing subtree had open, which decides what the block's own clips were cut against. */
    FRectangle CapturedClipRectangle;

    /** @brief The epoch the block recorded in, which has to still be current for the pointers inside it to mean anything. */
    uint64 CapturedEpoch;

    /** @brief The frame the block was last recorded or replayed on, which is the order the budget evicts in. */
    uint64 LastUsedFrame;

    /** @brief The DPI scale the element was drawn at, which every pixel position in the block was rounded to. */
    float CapturedScale;

    /** @brief The layer the element was asked to draw on, which every recorded layer counts from. */
    int32 BaseLayerId;

    /** @brief The layer the element reported back, which the caller uses to place whatever follows. */
    int32 MaxLayerId;

    /** @brief How deep the clip stack was on entry, so an unbalanced push cannot be replayed into the wrong place. */
    int32 CapturedClipDepth;

    /** @brief How many text commands the block holds, which keeps the ordinal text cache aligned when they are skipped. */
    int32 TextCommandCount;

    /** @brief True once the block holds a complete recording, and false while it is an empty shell kept for reuse. */
    bool bValid;

    /** @brief True once the block also holds the geometry those commands tessellated into. */
    bool bGeometryValid;

    /** @brief False once the block's commands have been seen split apart by the layer sort, which geometry splicing cannot survive. */
    bool bGeometryContiguous;

    /** @brief True when a clip was open around the element as it recorded. */
    bool bHasCapturedClip;
};

static_assert(sizeof(FDrawCacheBlock) == 248, "FDrawCacheBlock grew; every cached subtree pays for it");

struct APPLICATION_API DrawCacheRegistry
{
    /** @brief Starts tracking a block. */
    static void Register(FDrawCacheBlock* Block);

    /** @brief Stops tracking a block, which its destructor does. */
    static void Unregister(FDrawCacheBlock* Block);

    /** @brief Notes that a block changed size, so the running total stays right. */
    static void NotifySizeChanged(int64 ByteDelta);

    /** @brief Releases the blocks that have gone longest without being used until the total is under budget. */
    static void EnforceBudget();

    /** @brief Drops every block's contents, which a device loss or a style rebuild wants. */
    static void ReleaseAll();

    /** @return How many bytes every live block holds between them. */
    NODISCARD static int64 GetTotalByteSize();

    /** @return How many blocks are live. */
    NODISCARD static int32 GetBlockCount();

    /** @return The most memory live blocks have held at once, which is what the budget has to cover. */
    NODISCARD static int64 GetPeakByteSize();

    /** @brief Logs how many subtrees are cached, how many commands they replay, and what that costs. */
    static void DumpStats();

    /** @brief Advances the frame counter the eviction order is measured in. */
    static void BeginFrame();

    /** @return The frame number blocks stamp themselves with when used. */
    NODISCARD static uint64 GetCurrentFrame();
};
