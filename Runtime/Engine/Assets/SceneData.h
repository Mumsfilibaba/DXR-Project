#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Engine/EngineModule.h"
#include "Engine/Assets/TextureResource.h"
#include "Engine/Assets/VertexFormat.h"
#include "Engine/Resources/Material.h"
#include "RHI/RHITypes.h"

struct FMeshPartition
{
    FMeshPartition()
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

struct FMeshData
{
    void Clear()
    {
        Vertices.Clear();
        Indices.Clear();
        Partitions.Clear();
    }

    bool Hasdata() const
    {
        return !Vertices.IsEmpty();
    }

    void ShrinkBuffers()
    {
        Vertices.Shrink();
        Indices.Shrink();
        Partitions.Shrink();
    }

    int32 GetVertexCount() const
    {
        return Vertices.Size();
    }

    int32 GetIndexCount() const
    {
        return Indices.Size();
    }

    int32 GetSubMeshCount() const
    {
        return Partitions.Size();
    }
    
    /** @brief - Name of the mesh specified in the model-file */
    FString Name;
    
    TArray<FVertex>        Vertices;
    TArray<uint32>         Indices;
    TArray<FMeshPartition> Partitions;
};

struct FImportedMaterial
{
    /** @brief - Name of the material specified in the model-file */
    FString Name;

    /** @brief - Diffuse texture */
    FTexture2DRef DiffuseTexture;

    /** @brief - Normal texture */
    FTexture2DRef NormalTexture;

    /** @brief - Specular texture - Stores AO, Metallic, and Roughness in the same textures */
    FTexture2DRef SpecularTexture;

    /** @brief - Emissive texture */
    FTexture2DRef EmissiveTexture;

    /** @brief - AO texture - Ambient Occlusion */
    FTexture2DRef AOTexture;

    /** @brief - Roughness texture*/
    FTexture2DRef RoughnessTexture;

    /** @brief - Metallic Texture*/
    FTexture2DRef MetallicTexture;

    /** @brief - Metallic Texture*/
    FTexture2DRef AlphaMaskTexture;

    /** @brief - Diffuse Parameter */
    FVector3 Diffuse;

    /** @brief - AO Parameter */
    float AO = 1.0f;

    /** @brief - Roughness Parameter */
    float Roughness = 1.0f;

    /** @brief - Metallic Parameter */
    float Metallic = 1.0f;

    /** @brief - Flags containing material-properties */
    EMaterialFlags MaterialFlags = EMaterialFlags::None;
};

struct ENGINE_API FImportedModel
{
    void AddToWorld(class FWorld* World);

    bool HasData() const
    {
        return !Models.IsEmpty() && !Materials.IsEmpty();
    }

    bool HasModelData() const
    {
        return !Models.IsEmpty();
    }

    bool HasMaterialData() const
    {
        return !Materials.IsEmpty();
    }

    TArray<FMeshData>         Models;
    TArray<FImportedMaterial> Materials;

     /** @brief - A scale used to scale each actor when using add to scene */
    float Scale = 1.0f;
};
