#pragma once
#include "VertexFormat.h"
#include "TextureResource.h"

#include "Engine/EngineModule.h"

#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"

#include "RHI/RHITypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshData

struct FMeshData
{
    FMeshData() = default;

    FMeshData(FMeshData&&) = default;
    FMeshData(const FMeshData&) = default;

    FMeshData& operator=(FMeshData&&) = default;
    FMeshData& operator=(const FMeshData&) = default;

    inline void Clear()
    {
        Vertices.Clear();
        Indices.Clear();
    }

    inline bool Hasdata() const
    {
        return !Vertices.IsEmpty();
    }

    inline void RefitContainers()
    {
        Vertices.Shrink();
        Indices.Shrink();
    }

    TArray<FVertex> Vertices;
    TArray<uint32>  Indices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FModelData

struct FModelData
{
    FModelData() = default;

    FModelData(FModelData&&) = default;
    FModelData(const FModelData&) = default;

    FModelData& operator=(FModelData&&) = default;
    FModelData& operator=(const FModelData&) = default;

     /** @brief: Name of the mesh specified in the model-file */
    FString Name;

     /** @brief: Model mesh data */
    FMeshData Mesh;

     /** @brief: The Material index in the FSceneData Materials Array */
    int32 MaterialIndex = -1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMaterialData

struct FMaterialData
{
     /** @brief: Diffuse texture */
    FTextureResource2DRef DiffuseTexture;

     /** @brief: Normal texture */
    FTextureResource2DRef NormalTexture;

     /** @brief: Specular texture - Stores AO, Metallic, and Roughness in the same textures */
    FTextureResource2DRef SpecularTexture;

     /** @brief: Emissive texture */
    FTextureResource2DRef EmissiveTexture;

     /** @brief: AO texture - Ambient Occlusion */
    FTextureResource2DRef AOTexture;

     /** @brief: Roughness texture*/
    FTextureResource2DRef RoughnessTexture;

     /** @brief: Metallic Texture*/
    FTextureResource2DRef MetallicTexture;

     /** @brief: Metallic Texture*/
    FTextureResource2DRef AlphaMaskTexture;

     /** @brief: Diffuse Parameter */
    FVector3 Diffuse;

     /** @brief: AO Parameter */
    float AO = 1.0f;

     /** @brief: Roughness Parameter */
    float Roughness = 1.0f;

     /** @brief: Metallic Parameter */
    float Metallic = 1.0f;

    /** @brief: Is the diffuse and alpha stored in the same texture? */
    bool bAlphaDiffuseCombined = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FSceneData

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

     /** @brief: A scale used to scale each actor when using add to scene */
    float Scale = 1.0f;
};
