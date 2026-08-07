#pragma once
#include "Engine/EngineModule.h"
#include "Engine/Assets/MeshData.h"

struct ENGINE_API MeshFactory
{
    static FMeshData CreateCube(float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f) noexcept;
    static FMeshData CreatePlane(uint32 Width = 1, uint32 Height = 1) noexcept;
    static FMeshData CreateSphere(uint32 Subdivisions = 0, float Radius = 0.5f) noexcept;
    static FMeshData CreateCone(uint32 Sides = 16, float Radius = 0.5f, float Height = 1.0f) noexcept;
    static FMeshData CreateTorus(float RingRadius = 1.0f, float TubeRadius = 0.3f, uint32 RingSegments = 32, uint32 TubeSegments = 16) noexcept;
    static FMeshData CreateTeapot(uint32 Tessellation = 10) noexcept;
    static FMeshData CreatePyramid(float Width = 2.0f, float Depth = 2.0f, float Height = 2.0f) noexcept;
    static FMeshData CreateCylinder(uint32 Sides = 16, float Radius = 0.5f, float Height = 2.0f) noexcept;
};
