#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector2.h"
#include "Application/Draw/DrawTypes.h"

class FDrawCommandList;
class FFontAtlas;
class FRHITexture;

enum class EUIDrawBatchKind : uint8
{
    Textured = 0,
    Shape    = 1,
    Text     = 2,
};

struct FUIVertex
{
    FUIVertex()
        : Position()
        , TexCoord()
        , Color(0)
    {
    }

    Vector2 Position;
    Vector2 TexCoord;
    uint32  Color;
};

struct FUIShapeInstance
{
    Vector2 Position;
    uint32  Color;
    Vector2 DrawSize;
    Vector2 LocalOrigin;
    Vector2 RectSize;
    float   RadiusTL;
    float   RadiusTR;
    float   RadiusBR;
    float   RadiusBL;
    float   Thickness;
    float   ShapeKind;
};

struct FUITextGlyphInstance
{
    Vector2 Position;
    Vector2 Size;
    Vector2 MinTexCoord;
    Vector2 MaxTexCoord;
    uint32  Color;
};

static_assert(sizeof(FUIVertex) == 20, "FUIVertex no longer matches its input layout");
static_assert(sizeof(FUIShapeInstance) == 60, "FUIShapeInstance no longer matches its input layout");
static_assert(sizeof(FUITextGlyphInstance) == 36, "FUITextGlyphInstance no longer matches its input layout");

struct FDrawCacheBlock;

class FUITextureHandle
{
public:
    FUITextureHandle()
        : Bits(0)
    {
    }

    explicit FUITextureHandle(const FFontAtlas* InAtlas)
        : Bits(InAtlas ? (reinterpret_cast<uintptr_t>(InAtlas) | AtlasTag) : 0)
    {
        CHECK((reinterpret_cast<uintptr_t>(InAtlas) & AtlasTag) == 0);
    }

    explicit FUITextureHandle(FRHITexture* InTexture)
        : Bits(reinterpret_cast<uintptr_t>(InTexture))
    {
        CHECK((Bits & AtlasTag) == 0);
    }

    /** @return The glyph atlas sampled, uploaded lazily by the renderer, or null. */
    NODISCARD FORCEINLINE const FFontAtlas* GetAtlas() const
    {
        return (Bits & AtlasTag) ? reinterpret_cast<const FFontAtlas*>(Bits & ~AtlasTag) : nullptr;
    }

    /** @return The texture sampled, or null. */
    NODISCARD FORCEINLINE FRHITexture* GetTexture() const
    {
        return (Bits & AtlasTag) ? nullptr : reinterpret_cast<FRHITexture*>(Bits);
    }

    /** @return True when neither an atlas nor a texture is set, which is the untextured white-texel case. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Bits == 0;
    }

    NODISCARD FORCEINLINE bool operator==(const FUITextureHandle& Other) const
    {
        return Bits == Other.Bits;
    }

    NODISCARD FORCEINLINE bool operator!=(const FUITextureHandle& Other) const
    {
        return !(*this == Other);
    }

private:
    static constexpr uintptr_t AtlasTag = 1;

    uintptr_t Bits;
};

static_assert(sizeof(FUITextureHandle) == sizeof(void*), "FUITextureHandle is meant to cost one pointer");

struct FUIDrawBatch
{
    FUIDrawBatch()
        : ScissorRectangle()
        , Texture()
        , IndexOffset(0)
        , IndexCount(0)
        , Kind(EUIDrawBatchKind::Textured)
        , bIsClipped(false)
    {
    }

    FRectangle       ScissorRectangle;
    FUITextureHandle Texture;
    int32            IndexOffset;
    int32            IndexCount;
    EUIDrawBatchKind Kind;
    bool             bIsClipped;
};

static_assert(sizeof(FUIDrawBatch) == 40, "FUIDrawBatch grew; every recorded and cached batch pays for it");

class APPLICATION_API FUIDrawData
{
public:

    /** @brief The number of vertices one draw data can hold. The indices are 32-bit, so this is a budget rather than a format limit. */
    static constexpr int32 MaxVertexCount = 262144;

    /** @brief The fewest segments a rounded corner is tessellated into. */
    static constexpr int32 MinCornerSegments = 2;

    /** @brief The most segments a rounded corner is tessellated into, whatever its radius. */
    static constexpr int32 MaxCornerSegments = 12;

    /** @brief The fewest columns a rounded bottom bar is tessellated into. */
    static constexpr int32 MinBarColumns = 8;

    /** @brief The most columns a rounded bottom bar is tessellated into, whatever its width. */
    static constexpr int32 MaxBarColumns = 64;

    /** @brief How far a miter join may stretch past the stroke half-width before it is cut back. */
    static constexpr float MiterLimit = 4.0f;

    /** @brief How wide the soft edge laid over a curved silhouette is, in pixels. */
    static constexpr float FringeWidth = 1.0f;

    static constexpr float ShapeKindFill   = 0.0f;
    static constexpr float ShapeKindStroke = 1.0f;
    static constexpr float ShapeKindWedge  = 2.0f;

public:
    FUIDrawData();
    ~FUIDrawData();

    /**
     * @brief Translates a command list into triangles, in layer order.
     *
     * @param CommandList The list to translate.
     */
    void BuildFromCommandList(const FDrawCommandList& CommandList);

    /** @brief Drops every vertex, index and batch so the data can be built again. */
    void Reset();

    /**
     * @brief Turns the soft edge on curved geometry on or off.
     *
     * @param bEnabled True to emit the fringe, false to emit the bare silhouette.
     */
    void SetAntiAliasingEnabled(bool bEnabled);

    /** @return True while curved geometry is emitted with a soft edge. */
    NODISCARD FORCEINLINE bool IsAntiAliasingEnabled() const
    {
        return bAntiAliasingEnabled;
    }

    NODISCARD FORCEINLINE const TArray<FUIVertex>& GetVertices() const
    {
        return Vertices;
    }

    NODISCARD FORCEINLINE const TArray<uint32>& GetIndices() const
    {
        return Indices;
    }

    NODISCARD FORCEINLINE const TArray<FUIShapeInstance>& GetShapeInstances() const
    {
        return ShapeInstances;
    }

    NODISCARD FORCEINLINE const TArray<FUITextGlyphInstance>& GetTextGlyphInstances() const
    {
        return TextGlyphInstances;
    }

    NODISCARD FORCEINLINE const TArray<FUIDrawBatch>& GetBatches() const
    {
        return Batches;
    }

    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Batches.IsEmpty();
    }

    /**
     * @return True when the whole build was spliced from cached blocks, so nothing was tessellated afresh
     * and the streams are the same bytes as the last frame that replayed the same blocks in the same order.
     */
    NODISCARD FORCEINLINE bool IsFullyReplayed() const
    {
        return bFullyReplayedGeometry && !Batches.IsEmpty();
    }

    /** @return How many text commands' glyphs the draw data keeps between builds. */
    NODISCARD FORCEINLINE int32 GetTextGeometryCacheSize() const
    {
        return TextGeometryCache.Size();
    }

    /** @return What the replayed blocks were and how big they were, which stands in for hashing the streams. */
    NODISCARD FORCEINLINE uint64 GetReplayFingerprint() const
    {
        return ReplayFingerprint;
    }

private:
    struct FTextGeometryCacheEntry
    {
        uint64 AtlasRevision = 0;
        String Text;
        const IFontFace* Font = nullptr;
        FRectangle Bounds;
        uint32 PackedColor = 0;
        bool bValid = false;
        TArray<FUITextGlyphInstance> Instances;
    };

    struct FDrawCacheGeometryMarker
    {
        int32 VertexBase;
        int32 IndexBase;
        int32 ShapeInstanceBase;
        int32 TextInstanceBase;
        int32 BatchBase;
        int32 OpenBatchIndexCount;
    };

    NODISCARD int32 ProcessDrawCacheSpan(int32 SortedIndex, const FDrawCommandList& CommandList);

    void BeginDrawCacheGeometry(FDrawCacheGeometryMarker& OutMarker) const;
    void CaptureDrawCacheGeometry(const FDrawCacheGeometryMarker& Marker, FDrawCacheBlock& Block) const;
    void AppendDrawCacheGeometry(const FDrawCacheBlock& Block);

    NODISCARD static int32 GetStreamBase(const FDrawCacheGeometryMarker& Marker, EUIDrawBatchKind Kind);

    void TessellateCommand(const FDrawCommand& Command, const FDrawCommandList& CommandList);

    NODISCARD static FRectangle ComputePointBounds(TArrayView<const Vector2> Points, float Thickness);
    NODISCARD static float ComputeWindingSign(TArrayView<const Vector2> Points);

    static void BuildRoundedBoxOutline(const FRectangle& Bounds, const FCornerRadii& Radius, TArray<Vector2>& OutPoints, float Inset = 0.0f);
    static void BuildMiterOffsets(TArrayView<const Vector2> Points, bool bClosed, TArray<Vector2>& OutOffsets);

    NODISCARD bool IsCulledByClip(const FRectangle& Bounds) const;

    FUIDrawBatch& GetOrOpenBatch(const FUITextureHandle& Texture, EUIDrawBatchKind Kind = EUIDrawBatchKind::Textured);

    void ApplyCommandClip(const FDrawCommand& Command, const FDrawCommandList& CommandList);
    void AddBox(const FDrawCommand& Command);
    void AddBoxOutline(const FDrawCommand& Command);
    void AddText(const FDrawCommand& Command, int32 CacheIndex);
    void AddImage(const FDrawCommand& Command);
    void AddRoundedBottomBar(const FDrawCommand& Command);
    void AddRoundedAccentRing(const FDrawCommand& Command);
    void AddCornerWedges(const FDrawCommand& Command);
    void TrimTextGeometryCache();
    void AddPolyline(TArrayView<const Vector2> Points, float Thickness, bool bClosed, uint32 PackedColor);
    void AddConvexPolygon(TArrayView<const Vector2> Points, uint32 PackedColor);
    void AddQuad(const FRectangle& Bounds, const Vector2& MinTexCoord, const Vector2& MaxTexCoord, uint32 PackedColor);
    void AddRoundedBox(const FRectangle& Bounds, const FCornerRadii& Radius, uint32 PackedColor,
        const Vector2& MinTexCoord = Vector2(0.0f, 0.0f), const Vector2& MaxTexCoord = Vector2(1.0f, 1.0f));
    void AddSdfRoundedQuad(const FRectangle& Bounds, const FCornerRadii& Radius, uint32 PackedColor, float Thickness, float ShapeKind);
    void EmplaceVertex(const Vector2& Position, uint32 PackedColor);
    void EmplaceFillVertex(const Vector2& Position, const FRectangle& Bounds, uint32 PackedColor,
        const Vector2& MinTexCoord = Vector2(0.0f, 0.0f), const Vector2& MaxTexCoord = Vector2(1.0f, 1.0f));

    TArray<FUIVertex>               Vertices;
    TArray<uint32>                  Indices;
    TArray<FUIShapeInstance>        ShapeInstances;
    TArray<FUITextGlyphInstance>    TextGlyphInstances;
    TArray<FUIDrawBatch>            Batches;
    TArray<Vector2>                 ScratchPoints;
    TArray<Vector2>                 ScratchOffsets;
    TArray<int32>                   SortedCommandIndices;
    TArray<int32>                   SortScratchIndices;
    TArray<FTextGeometryCacheEntry> TextGeometryCache;
    uint64                          ReplayFingerprint;
    const FDrawCommandList*         SourceCommandList;
    FRectangle                      ActiveClipRectangle;
    int32                           TextCommandOrdinal;
    uint16                          ActiveClipId;
    uint8                           TextCacheOversizedFrames;
    bool                            bFullyReplayedGeometry;
    bool                            bHasActiveClip;
    bool                            bAntiAliasingEnabled;
};
