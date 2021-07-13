#pragma once
#include "Core.h"
#include "VertexFormat.h"

#include "Core/Containers/String.h"

struct MeshData
{
    TArray<Vertex> Vertices;
    TArray<uint32> Indices;
};

class MeshFactory
{
public:
    static MeshData CreateFromFile( const String& Filename, bool LeftHanded = false ) noexcept;
    static MeshData CreateCube( float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f ) noexcept;
    static MeshData CreatePlane( uint32 Width = 1, uint32 Height = 1 ) noexcept;
    static MeshData CreateSphere( uint32 Subdivisions = 0, float Radius = 0.5f ) noexcept;
    static MeshData CreateCone( uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f ) noexcept;
    //static MeshData createTorus() noexcept;
    //static MeshData createTeapot() noexcept;
    static MeshData CreatePyramid() noexcept;
    static MeshData CreateCylinder( uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f ) noexcept;
};