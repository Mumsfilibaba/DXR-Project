#pragma once
#include <Core/Containers/Array.h>
#include <Core/Math/Vector2.h>
#include <Application/Draw/UIDrawData.h>

struct FUIShapeVertex
{
    Vector2 Position;
    uint32  Color;
    Vector2 LocalPos;
    Vector2 RectSize;
    float   RadiusTL;
    float   RadiusTR;
    float   RadiusBR;
    float   RadiusBL;
    float   Thickness;
    float   ShapeKind;
};

namespace UITestUtils
{
    /**
     * @brief Expands every instanced shape the draw data holds into four vertices and six indices, as the shape vertex
     * shader would, so a test can inspect or rasterize them on the CPU.
     */
    void ExpandShapeInstances(const FUIDrawData& DrawData, TArray<FUIShapeVertex>& OutVertices, TArray<uint32>& OutIndices);
}

/** @return The draw data's shape instances expanded into vertices, four per shape. */
NODISCARD TArray<FUIShapeVertex> GetShapeVertices(const FUIDrawData& DrawData);

/** @return The indices over GetShapeVertices, six per shape. */
NODISCARD TArray<uint32> GetShapeIndices(const FUIDrawData& DrawData);
