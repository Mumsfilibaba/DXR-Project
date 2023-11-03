#pragma once
#include "VertexFormat.h"
#include "TextureResource.h"
#include "Engine/EngineModule.h"
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "RHI/RHITypes.h"

struct FMeshData
{
    FORCEINLINE void Clear()
    {
        Vertices.Clear();
        Indices.Clear();
    }

    FORCEINLINE bool Hasdata() const
    {
        return !Vertices.IsEmpty();
    }

    FORCEINLINE void ShrinkBuffers()
    {
        Vertices.Shrink();
        Indices.Shrink();
    }

    FORCEINLINE int32 GetVertexCount() const
    {
        return Vertices.Size();
    }

    FORCEINLINE int32 GetIndexCount() const
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

    /** @brief - Is the diffuse and alpha stored in the same texture? */
    bool bAlphaDiffuseCombined = false;

    /** @brief - Should this material have culling turned off ? */
    bool bIsDoubleSided = false;
};


struct ENGINE_API FSceneData
{
    void AddToScene(class FScene* Scene);

    FORCEINLINE bool HasData() const
    {
        return !Models.IsEmpty() && !Materials.IsEmpty();
    }

    FORCEINLINE bool HasModelData() const
    {
        return !Models.IsEmpty();
    }

    FORCEINLINE bool HasMaterialData() const
    {
        return !Materials.IsEmpty();
    }

    TArray<FModelData>    Models;
    TArray<FMaterialData> Materials;

     /** @brief - A scale used to scale each actor when using add to scene */
    float Scale = 1.0f;
};
