#pragma once
#include "SceneData.h"

#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMeshUtilities

class ENGINE_API CMeshUtilities
{
public:
    static void Subdivide(SMeshData& OutData, uint32 Subdivisions = 1) noexcept;
    static void Optimize(SMeshData& OutData, uint32 StartVertex = 0) noexcept;

    static void CalculateHardNormals(SMeshData& OutData) noexcept;
    static void CalculateSoftNormals(SMeshData& OutData) noexcept;
    static void CalculateTangents(SMeshData& OutData) noexcept;

    static void ReverseHandedness(SMeshData& OutData) noexcept;
};