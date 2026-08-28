#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Math/Vector2.h"
#include "Application/Draw/DrawTypes.h"

class FDrawCommandList;
class FFontAtlas;
class FRHITexture;

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

struct FUITextureHandle
{
    FUITextureHandle()
        : Atlas(nullptr)
        , Texture(nullptr)
    {
    }

    explicit FUITextureHandle(const FFontAtlas* InAtlas)
        : Atlas(InAtlas)
        , Texture(nullptr)
    {
    }

    explicit FUITextureHandle(FRHITexture* InTexture)
        : Atlas(nullptr)
        , Texture(InTexture)
    {
    }

    /** @return True when neither an atlas nor a texture is set, which is the untextured white-texel case. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Atlas == nullptr && Texture == nullptr;
    }

    NODISCARD FORCEINLINE bool operator==(const FUITextureHandle& Other) const
    {
        return Atlas == Other.Atlas && Texture == Other.Texture;
    }

    NODISCARD FORCEINLINE bool operator!=(const FUITextureHandle& Other) const
    {
        return !(*this == Other);
    }

    /** @brief The glyph atlas sampled, uploaded lazily by the renderer, or null. */
    const FFontAtlas* Atlas;

    /** @brief The texture sampled, or null. */
    FRHITexture* Texture;
};

struct FUIDrawBatch
{
    FUIDrawBatch()
        : ScissorRectangle()
        , Texture()
        , IndexOffset(0)
        , IndexCount(0)
        , bIsClipped(false)
    {
    }

    FRectangle       ScissorRectangle;
    FUITextureHandle Texture;
    int32            IndexOffset;
    int32            IndexCount;
    bool             bIsClipped;
};

class APPLICATION_API FUIDrawData
{
public:

    /** @brief The number of vertices one draw data can hold. The indices are 32-bit, so this is a budget rather than a format limit. */
    static constexpr int32 MaxVertexCount = 262144;

    /** @brief The fewest segments a rounded corner is tessellated into. */
    static constexpr int32 MinCornerSegments = 2;

    /** @brief The most segments a rounded corner is tessellated into, whatever its radius. */
    static constexpr int32 MaxCornerSegments = 12;

    /** @brief How far a miter join may stretch past the stroke half-width before it is cut back. */
    static constexpr float MiterLimit = 4.0f;

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

    NODISCARD FORCEINLINE const TArray<FUIVertex>& GetVertices() const
    {
        return Vertices;
    }

    NODISCARD FORCEINLINE const TArray<uint32>& GetIndices() const
    {
        return Indices;
    }

    NODISCARD FORCEINLINE const TArray<FUIDrawBatch>& GetBatches() const
    {
        return Batches;
    }

    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Batches.IsEmpty();
    }

private:
    NODISCARD static FRectangle ComputePointBounds(TArrayView<const Vector2> Points, float Thickness);
    static void BuildRoundedBoxOutline(const FRectangle& Bounds, const FCornerRadii& Radius, TArray<Vector2>& OutPoints);

    NODISCARD bool IsCulledByClip(const FRectangle& Bounds) const;

    void AddBox(const FDrawCommand& Command);
    void AddBoxOutline(const FDrawCommand& Command);
    void AddText(const FDrawCommand& Command);
    void AddImage(const FDrawCommand& Command);
    void AddPolyline(TArrayView<const Vector2> Points, float Thickness, bool bClosed, uint32 PackedColor);
    void AddConvexPolygon(TArrayView<const Vector2> Points, uint32 PackedColor);
    void AddQuad(const FRectangle& Bounds, const Vector2& MinTexCoord, const Vector2& MaxTexCoord, uint32 PackedColor);
    void AddRoundedBox(const FRectangle& Bounds, const FCornerRadii& Radius, uint32 PackedColor);

    FUIDrawBatch& GetOrOpenBatch(const FUITextureHandle& Texture);

    TArray<FUIVertex>    Vertices;
    TArray<uint32>       Indices;
    TArray<FUIDrawBatch> Batches;
    TArray<FRectangle>   ClipStack;
    TArray<Vector2>      ScratchPoints;
};
