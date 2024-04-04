#pragma once
#include "VertexFormat.h"
#include "TextureResource.h"
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Engine/EngineModule.h"
#include "Engine/Resources/Material.h"
#include "RHI/RHITypes.h"

struct FMeshData
{
    void Clear()
    {
        Vertices.Clear();
        Indices.Clear();
    }

    bool Hasdata() const
    {
        return !Vertices.IsEmpty();
    }

    void ShrinkBuffers()
    {
        Vertices.Shrink();
        Indices.Shrink();
    }

    int32 GetVertexCount() const
    {
        return Vertices.Size();
    }

    int32 GetIndexCount() const
    {
        return Indices.Size();
    }

    TArray<FVertex> Vertices;
    TArray<uint32>  Indices;
};


struct FModelData
{
    /** @brief - Name of the mesh specified in the model-file */
    FString Name;

    /** @brief - Model mesh data */
    FMeshData Mesh;

    /** @brief - The Material index in the FSceneData Materials Array */
    int32 MaterialIndex = -1;
};

struct FMaterialData
{
    /** @brief - Name of the material specified in the model-file */
    FString Name;

    /** @brief - Diffuse texture */
    FTextureResource2DRef DiffuseTexture;

    /** @brief - Normal texture */
    FTextureResource2DRef NormalTexture;

    /** @brief - Specular texture - Stores AO, Metallic, and Roughness in the same textures */
    FTextureResource2DRef SpecularTexture;

    /** @brief - Emissive texture */
    FTextureResource2DRef EmissiveTexture;

    /** @brief - AO texture - Ambient Occlusion */
    FTextureResource2DRef AOTexture;

    /** @brief - Roughness texture*/
    FTextureResource2DRef RoughnessTexture;

    /** @brief - Metallic Texture*/
    FTextureResource2DRef MetallicTexture;

    /** @brief - Metallic Texture*/
    FTextureResource2DRef AlphaMaskTexture;

    /** @brief - Diffuse Parameter */
    FVector3 Diffuse;

    /** @brief - AO Parameter */
    float AO = 1.0f;

    /** @brief - Roughness Parameter */
    float Roughness = 1.0f;

    /** @brief - Metallic Parameter */
    float Metallic = 1.0f;

    /** @brief - Flags containing material-properties */
    EMaterialFlags MaterialFlags = MaterialFlag_None;
};


struct ENGINE_API FSceneData
{
    void AddToScene(class FScene* Scene);

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

    TArray<FModelData>    Models;
    TArray<FMaterialData> Materials;

     /** @brief - A scale used to scale each actor when using add to scene */
    float Scale = 1.0f;
};
