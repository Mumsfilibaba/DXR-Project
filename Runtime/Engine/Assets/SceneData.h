#pragma once
#include "VertexFormat.h"

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
    // C++ Being retarded?
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
        Vertices.ShrinkToFit();
        Indices.ShrinkToFit();
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

typedef TSharedPtr<struct FImage2D> FImage2DPtr;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FImage2D - Image for when loading materials

struct FImage2D
{
    FImage2D() = default;

    FImage2D(const FString& InPath, uint16 InWidth, uint16 InHeight, EFormat InFormat)
        : Path(InPath)
        , Image()
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
    { }

     /** @brief: Relative path to the image specified in the model-file */
    FString Path;

     /** @brief: Pointer to image data */
    TUniquePtr<uint8[]> Image;

     /** @brief: Size of the image */
    uint16 Width = 0;
    uint16 Height = 0;

     /** @brief: The format that the image was loaded as */
    EFormat Format = EFormat::Unknown;

    bool bIsLoaded = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMaterialData

struct FMaterialData
{
     /** @brief: Diffuse texture */
    int8 DiffuseTexture = -1;

     /** @brief: Normal texture */
    int8 NormalTexture = -1;

     /** @brief: Specular texture - Stores AO, Metallic, and Roughness in the same textures */
    int8 SpecularTexture = -1;

     /** @brief: Emissive texture */
    int8 EmissiveTexture = -1;

     /** @brief: AO texture - Ambient Occlusion */
    int8 AOTexture = -1;

     /** @brief: Roughness texture*/
    int8 RoughnessTexture = -1;

     /** @brief: Metallic Texture*/
    int8 MetallicTexture = -1;

     /** @brief: Metallic Texture*/
    int8 AlphaMaskTexture = -1;

     /** @brief: Diffuse Parameter */
    FVector3 Diffuse;

     /** @brief: AO Parameter */
    float AO = 1.0f;

     /** @brief: Roughness Parameter */
    float Roughness = 1.0f;

     /** @brief: Metallic Parameter */
    float Metallic = 1.0f;
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
    TArray<FImage2DPtr>   Textures;

     /** @brief: A scale used to scale each actor when using add to scene */
    float Scale = 1.0f;
};
