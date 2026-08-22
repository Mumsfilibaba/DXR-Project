#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector2.h"
#include "Application/Draw/DrawTypes.h"

class FDrawCommandList;
class FFontAtlas;

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

struct FUIDrawBatch
{
    FUIDrawBatch()
        : ScissorRectangle()
        , Atlas(nullptr)
        , IndexOffset(0)
        , IndexCount(0)
        , bIsClipped(false)
    {
    }

    FRectangle        ScissorRectangle;
    const FFontAtlas* Atlas;
    int32             IndexOffset;
    int32             IndexCount;
    bool              bIsClipped;
};

class APPLICATION_API FUIDrawData
{
public:

    /** @brief The number of vertices one draw data can hold, limited by the 16-bit index buffer. */
    static constexpr int32 MaxVertexCount = 65536;

    /** @brief The fewest segments a rounded corner is tessellated into. */
    static constexpr int32 MinCornerSegments = 2;

    /** @brief The most segments a rounded corner is tessellated into, whatever its radius. */
    static constexpr int32 MaxCornerSegments = 12;

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

    NODISCARD FORCEINLINE const TArray<uint16>& GetIndices() const
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

    // True when a clip region is open and the bounds fall entirely outside it
    NODISCARD bool IsCulledByClip(const FRectangle& Bounds) const;

    void          AddBox(const FDrawCommand& Command);
    void          AddText(const FDrawCommand& Command);
    void          AddQuad(const FRectangle& Bounds, const Vector2& MinTexCoord, const Vector2& MaxTexCoord, uint32 PackedColor);
    void          AddRoundedBox(const FRectangle& Bounds, float Radius, uint32 PackedColor);
    FUIDrawBatch& GetOrOpenBatch(const FFontAtlas* Atlas);

    TArray<FUIVertex>    Vertices;
    TArray<uint16>       Indices;
    TArray<FUIDrawBatch> Batches;
    TArray<FRectangle>   ClipStack;
};
