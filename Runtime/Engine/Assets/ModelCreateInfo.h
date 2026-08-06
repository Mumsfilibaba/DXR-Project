#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Engine/EngineModule.h"
#include "Engine/Assets/VertexFormat.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Texture.h"
#include "RendererCore/VertexDeclaration.h"
#include "RHI/RHITypes.h"

struct EMaterialTexture
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
};

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

struct ENGINE_API FMeshCreateInfo
{
    FMeshCreateInfo();
    FMeshCreateInfo(const FMeshCreateInfo& Other);
    FMeshCreateInfo(FMeshCreateInfo&& Other);
    ~FMeshCreateInfo();

    FMeshCreateInfo& operator=(const FMeshCreateInfo& Other);
    FMeshCreateInfo& operator=(FMeshCreateInfo&& Other);

    void Subdivide(uint32 Subdivisions = 1);
    void Optimize(uint32 StartVertex = 0);

    void CalculateHardNormals();
    void CalculateSoftNormals();
    void CalculateTangents();
    void CalculateTangentSigns();
    void SplitTangentSeams();

    void ValidateTangents();
    void ReverseHandedness();
    void InvertAxisX();

    bool PackVertexStreams(TArray<uint8> (&OutStreams)[VERTEX_MAX_STREAMS]) const;

    TArray<uint16> GetSmallIndices() const;

    String                Name;
    TArray<FSubMeshInfo>  SubMeshes;
    TArray<uint32>        Indices;
    TArray<FSourceVertex> Vertices;
    FVertexDeclaration    Declaration;
    TArray<uint8>         PackedStreams[VERTEX_MAX_STREAMS];
    int32                 PackedVertexCount;
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

    String         Name;
    FTexture2DRef  Textures[EMaterialTexture::Count];
    Vector3        Diffuse;
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

struct ENGINE_API MeshFactory
{
    static FMeshCreateInfo CreateCube(float Width = 1.0f, float Height = 1.0f, float Depth = 1.0f) noexcept;
    static FMeshCreateInfo CreatePlane(uint32 Width = 1, uint32 Height = 1) noexcept;
    static FMeshCreateInfo CreateSphere(uint32 Subdivisions = 0, float Radius = 0.5f) noexcept;
    static FMeshCreateInfo CreateCone(uint32 Sides = 16, float Radius = 0.5f, float Height = 1.0f) noexcept;
    static FMeshCreateInfo CreateTorus(float RingRadius = 1.0f, float TubeRadius = 0.3f, uint32 RingSegments = 32, uint32 TubeSegments = 16) noexcept;
    static FMeshCreateInfo CreateTeapot(uint32 Tessellation = 10) noexcept;
    static FMeshCreateInfo CreatePyramid(float Width = 2.0f, float Depth = 2.0f, float Height = 2.0f) noexcept;
    static FMeshCreateInfo CreateCylinder(uint32 Sides = 16, float Radius = 0.5f, float Height = 2.0f) noexcept;
};
