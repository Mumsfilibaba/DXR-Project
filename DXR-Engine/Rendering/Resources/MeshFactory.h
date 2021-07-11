#pragma once
#include "Core.h"

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/MathHash.h"

struct Vertex
{
    CVector3 Position;
    CVector3 Normal;
    CVector3 Tangent;
    CVector2 TexCoord;

    FORCEINLINE bool operator==( const Vertex& Other ) const
    {
        return Position == (Other.Position) && (Normal == Other.Normal) && (Tangent == Other.Tangent) && (TexCoord == Other.TexCoord);
    }

    FORCEINLINE bool operator!=( const Vertex& Other ) const
    {
        return !(*this == Other);
    }
};

struct VertexHasher
{
    inline size_t operator()( const Vertex& v ) const
    {
        std::hash<CVector3> Hasher;

        size_t Hash = Hasher( v.Position );
        HashCombine<CVector3>( Hash, v.Normal );
        HashCombine<CVector3>( Hash, v.Tangent );
        HashCombine<CVector2>( Hash, v.TexCoord );

        return Hash;
    }
};

struct MeshData
{
    TArray<Vertex> Vertices;
    TArray<uint32> Indices;
};

class MeshFactory
{
public:
    static MeshData CreateFromFile( const std::string& Filename, bool LeftHanded = false ) noexcept;
    static MeshData CreateCube( float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f ) noexcept;
    static MeshData CreatePlane( uint32 Width = 1, uint32 Height = 1 ) noexcept;
    static MeshData CreateSphere( uint32 Subdivisions = 0, float Radius = 0.5f ) noexcept;
    static MeshData CreateCone( uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f ) noexcept;
    //static MeshData createTorus() noexcept;
    //static MeshData createTeapot() noexcept;
    static MeshData CreatePyramid() noexcept;
    static MeshData CreateCylinder( uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f ) noexcept;

    static void Subdivide( MeshData& OutData, uint32 Subdivisions = 1 ) noexcept;
    static void Optimize( MeshData& OutData, uint32 StartVertex = 0 ) noexcept;
    static void CalculateHardNormals( MeshData& OutData ) noexcept;
    static void CalculateTangents( MeshData& OutData ) noexcept;
};