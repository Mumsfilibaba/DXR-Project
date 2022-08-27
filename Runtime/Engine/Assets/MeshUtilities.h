#pragma once
#include "SceneData.h"

#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshUtilities

struct ENGINE_API FMeshUtilities
{
    static void Subdivide(FMeshData& OutData, uint32 Subdivisions = 1) noexcept;
    static void Optimize(FMeshData& OutData, uint32 StartVertex = 0) noexcept;

    static void CalculateHardNormals(FMeshData& OutData) noexcept;
    static void CalculateSoftNormals(FMeshData& OutData) noexcept;
    static void CalculateTangents(FMeshData& OutData) noexcept;

    static void ReverseHandedness(FMeshData& OutData) noexcept;
};