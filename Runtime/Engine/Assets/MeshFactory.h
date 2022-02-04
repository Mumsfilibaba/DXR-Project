#pragma once
#include "SceneData.h"

#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMeshFactory

class ENGINE_API CMeshFactory
{
public:
    static SMeshData CreateCube(float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f) noexcept;
    static SMeshData CreatePlane(uint32 Width = 1, uint32 Height = 1) noexcept;
    static SMeshData CreateSphere(uint32 Subdivisions = 0, float Radius = 0.5f) noexcept;
    static SMeshData CreateCone(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;
    //static SMeshData createTorus() noexcept;
    //static SMeshData createTeapot() noexcept;
    static SMeshData CreatePyramid() noexcept;
    static SMeshData CreateCylinder(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;
};