#include "UITestUtils.h"

void UITestUtils::ExpandShapeInstances(const FUIDrawData& DrawData, TArray<FUIShapeVertex>& OutVertices, TArray<uint32>& OutIndices)
{
    const TArray<FUIShapeInstance>& Instances = DrawData.GetShapeInstances();

    OutVertices.ResizeUninitialized(Instances.Size() * 4);
    OutIndices.ResizeUninitialized(Instances.Size() * 6);

    static const Vector2 Corners[4] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(1.0f, 0.0f),
        Vector2(1.0f, 1.0f),
        Vector2(0.0f, 1.0f),
    };

    for (int32 InstanceIndex = 0; InstanceIndex < Instances.Size(); ++InstanceIndex)
    {
        const FUIShapeInstance& Instance = Instances[InstanceIndex];

        FUIShapeVertex* Quad = OutVertices.Data() + (InstanceIndex * 4);
        for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
        {
            Quad[CornerIndex].Position  = Instance.Position + (Instance.DrawSize * Corners[CornerIndex]);
            Quad[CornerIndex].Color     = Instance.Color;
            Quad[CornerIndex].LocalPos  = Instance.LocalOrigin + (Instance.DrawSize * Corners[CornerIndex]);
            Quad[CornerIndex].RectSize  = Instance.RectSize;
            Quad[CornerIndex].RadiusTL  = Instance.RadiusTL;
            Quad[CornerIndex].RadiusTR  = Instance.RadiusTR;
            Quad[CornerIndex].RadiusBR  = Instance.RadiusBR;
            Quad[CornerIndex].RadiusBL  = Instance.RadiusBL;
            Quad[CornerIndex].Thickness = Instance.Thickness;
            Quad[CornerIndex].ShapeKind = Instance.ShapeKind;
        }

        const uint32 BaseVertex = static_cast<uint32>(InstanceIndex * 4);

        uint32* IndicesOut = OutIndices.Data() + (InstanceIndex * 6);
        IndicesOut[0] = BaseVertex + 0;
        IndicesOut[1] = BaseVertex + 1;
        IndicesOut[2] = BaseVertex + 2;
        IndicesOut[3] = BaseVertex + 0;
        IndicesOut[4] = BaseVertex + 2;
        IndicesOut[5] = BaseVertex + 3;
    }
}

TArray<FUIShapeVertex> GetShapeVertices(const FUIDrawData& DrawData)
{
    TArray<FUIShapeVertex> Vertices;
    TArray<uint32>         Indices;
    UITestUtils::ExpandShapeInstances(DrawData, Vertices, Indices);
    return Vertices;
}

TArray<uint32> GetShapeIndices(const FUIDrawData& DrawData)
{
    TArray<FUIShapeVertex> Vertices;
    TArray<uint32>         Indices;
    UITestUtils::ExpandShapeInstances(DrawData, Vertices, Indices);
    return Indices;
}
