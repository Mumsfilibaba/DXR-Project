#pragma once
#include "SceneData.h"

#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshFactory

struct ENGINE_API FMeshFactory
{
    static FMeshData CreateCube(float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f) noexcept;
    static FMeshData CreatePlane(uint32 Width = 1, uint32 Height = 1) noexcept;
    static FMeshData CreateSphere(uint32 Subdivisions = 0, float Radius = 0.5f) noexcept;
    static FMeshData CreateCone(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;
    //static FMeshData createTorus() noexcept;
    //static FMeshData createTeapot() noexcept;
    static FMeshData CreatePyramid() noexcept;
    static FMeshData CreateCylinder(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;

    static TArray<uint16> ConvertSmallIndices(const TArray<uint32>& Indicies) noexcept;
};