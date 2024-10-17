#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Engine/EngineModule.h"
#include "Engine/Assets/VertexFormat.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Texture.h"
#include "RHI/RHITypes.h"

namespace EMaterialTexture
{
    enum Type
    {
        Diffuse = 0,
        Normal,
        Specular,
        Emissive,
        AmbientOcclusion,
        Roughness,
        Metallic,
        AlphaMask,
        Count
    };
}

struct FSubMeshInfo
{
    FSubMeshInfo()
        : BaseVertex(0)
        , VertexCount(0)
        , StartIndex(0)
        , IndexCount(0)
        , MaterialIndex(-1)
    {
    }
    
    uint32 BaseVertex;
    uint32 VertexCount;
    uint32 StartIndex;
    uint32 IndexCount;
    int32  MaterialIndex;
};

struct FMeshCreateInfo
{
    FMeshCreateInfo()
        : Name()
        , SubMeshes()
        , Indices()
        , Vertices()
    {
    }

    void Subdivide(uint32 Subdivisions = 1);
    void Optimize(uint32 StartVertex = 0);

    void CalculateHardNormals();
    void CalculateSoftNormals();
    void CalculateTangents();

    void ReverseHandedness();

    TArray<uint16> GetSmallIndices() const;

    FString              Name;
    TArray<FSubMeshInfo> SubMeshes;
    TArray<uint32>       Indices;
    TArray<FVertex>      Vertices;
};

struct FMaterialCreateInfo
{
    FMaterialCreateInfo()
        : Name()
        , Textures()
        , Diffuse()
        , AmbientFactor(1.0f)
        , Roughness(1.0f)
        , Metallic()
    , MaterialFlags(EMaterialFlags::None)
    {
    }

    FString        Name;
    FTexture2DRef  Textures[EMaterialTexture::Count];
    FVector3       Diffuse;
    float          AmbientFactor;
    float          Roughness;
    float          Metallic;
    EMaterialFlags MaterialFlags;
};

struct FModelCreateInfo
{
    FModelCreateInfo()
        : Meshes()
        , Materials()
        , Scale(1.0f)
    {
    }

    TArray<FMeshCreateInfo>     Meshes;
    TArray<FMaterialCreateInfo> Materials;
    float                       Scale;
};

struct ENGINE_API FMeshFactory
{
    static FMeshCreateInfo CreateCube(float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f) noexcept;
    static FMeshCreateInfo CreatePlane(uint32 Width = 1, uint32 Height = 1) noexcept;
    static FMeshCreateInfo CreateSphere(uint32 Subdivisions = 0, float Radius = 0.5f) noexcept;
    static FMeshCreateInfo CreateCone(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;
    static FMeshCreateInfo CreateTorus() noexcept;
    //static FMeshCreateInfo createTeapot() noexcept;
    static FMeshCreateInfo CreatePyramid() noexcept;
    static FMeshCreateInfo CreateCylinder(uint32 Sides = 5, float Radius = 0.5f, float Height = 1.0f) noexcept;
};
