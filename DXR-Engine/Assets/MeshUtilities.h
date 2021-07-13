#pragma once
#include "Core.h"

class CMeshUtilities
{
public:
    static void Subdivide( MeshData& OutData, uint32 Subdivisions = 1 ) noexcept;
    static void Optimize( MeshData& OutData, uint32 StartVertex = 0 ) noexcept;
    static void CalculateHardNormals( MeshData& OutData ) noexcept;
    static void CalculateTangents( MeshData& OutData ) noexcept;
};