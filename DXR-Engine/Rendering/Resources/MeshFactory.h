#pragma once
#include "Core.h"

#include "Utilities/HashUtilities.h"

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT3 Tangent;
    XMFLOAT2 TexCoord;

    FORCEINLINE bool operator==(const Vertex& Other) const
    {
        return
            ((Position.x == Other.Position.x) && (Position.y == Other.Position.y) && (Position.z == Other.Position.z)) &&
            ((Normal.x   == Other.Normal.x)   && (Normal.y   == Other.Normal.y)   && (Normal.z   == Other.Normal.z))   &&
            ((Tangent.x  == Other.Tangent.x)  && (Tangent.y  == Other.Tangent.y)  && (Tangent.z  == Other.Tangent.z))  &&
            ((TexCoord.x == Other.TexCoord.x) && (TexCoord.y == Other.TexCoord.y));
    }
};

struct VertexHasher
{
    inline size_t operator()(const Vertex& V) const
    {
        std::hash<XMFLOAT3> Hasher;

        size_t Hash = Hasher(V.Position);
        HashCombine<XMFLOAT3>(Hash, V.Normal);
        HashCombine<XMFLOAT3>(Hash, V.Tangent);
        HashCombine<XMFLOAT2>(Hash, V.TexCoord);

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
    static MeshData CreateFromFile(const std::string& Filename, bool MergeMeshes = true, bool LeftHanded = true) noexcept;
    static MeshData CreateCube(float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f) noexcept;
    static MeshData CreatePlane(uint32 Width = 1, uint32 Height = 1) noexcept;
    static MeshData CreateSphere(uint32 Subdivisions = 0, float Radius = 0.5f) noexcept;
    static MeshData CreateCone(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;
    //static MeshData createTorus() noexcept;
    //static MeshData createTeapot() noexcept;
    static MeshData CreatePyramid() noexcept;
    static MeshData CreateCylinder(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;

    static void Subdivide(MeshData& OutData, uint32 Subdivisions = 1) noexcept;
    static void Optimize(MeshData& OutData, uint32 StartVertex = 0) noexcept;
    static void CalculateHardNormals(MeshData& OutData) noexcept;
    static void CalculateTangents(MeshData& OutData) noexcept;
};