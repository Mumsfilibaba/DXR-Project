#pragma once
#include "Engine/EngineModule.h"
#include "Engine/Assets/MeshData.h"

struct ENGINE_API MeshFactory
{
    NODISCARD static FMeshData CreateCube(float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f) noexcept;
    NODISCARD static FMeshData CreatePlane(uint32 Width = 1, uint32 Height = 1) noexcept;
    NODISCARD static FMeshData CreateSphere(uint32 Subdivisions = 0, float Radius = 0.5f) noexcept;
    NODISCARD static FMeshData CreateCone(uint32 Sides = 16, float Radius = 0.5f, float Height = 1.0f) noexcept;
    NODISCARD static FMeshData CreateTorus(float RingRadius = 1.0f, float TubeRadius = 0.3f, uint32 RingSegments = 32, uint32 TubeSegments = 16) noexcept;
    NODISCARD static FMeshData CreateTeapot(uint32 Tessellation = 10) noexcept;
    NODISCARD static FMeshData CreatePyramid(float Width = 2.0f, float Depth = 2.0f, float Height = 2.0f) noexcept;
    NODISCARD static FMeshData CreateCylinder(uint32 Sides = 16, float Radius = 0.5f, float Height = 2.0f) noexcept;
    NODISCARD static FMeshData CreateCapsule(uint32 Sides = 32, uint32 Rings = 8, float Radius = 0.5f, float Height = 2.0f) noexcept;
};
