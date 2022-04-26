#pragma once
#include "VertexFormat.h"

#include "Engine/EngineModule.h"

#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"

#include "RHI/TextureFormat.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// MeshData

struct SMeshData
{
    // C++ Being retarded?
    SMeshData() = default;

    SMeshData(SMeshData&&) = default;
    SMeshData(const SMeshData&) = default;

    SMeshData& operator=(SMeshData&&) = default;
    SMeshData& operator=(const SMeshData&) = default;

    inline void Clear()
    {
        Vertices.Clear();
        Indices.Clear();
    }

    inline bool Hasdata() const
    {
        return !Vertices.IsEmpty();
    }

    TArray<SVertex> Vertices;
    TArray<uint32> Indices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ModelData

struct SModelData
{
    SModelData() = default;

    SModelData(SModelData&&) = default;
    SModelData(const SModelData&) = default;

    SModelData& operator=(SModelData&&) = default;
    SModelData& operator=(const SModelData&) = default;

     /** @brief: Name of the mesh specified in the model-file */
    String Name;

     /** @brief: Model mesh data */
    SMeshData Mesh;

     /** @brief: The Material index in the SSceneData Materials Array */
    int32 MaterialIndex = -1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SImage2D - Image for when loading materials

struct SImage2D
{
    SImage2D() = default;

    SImage2D(const String& InPath, uint16 InWidth, uint16 InHeight, EFormat InFormat)
        : Path(InPath)
        , Image()
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
    { }

     /** @brief: Relative path to the image specified in the model-file */
    String Path;

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
// MaterialData

struct SMaterialData
{
     /** @brief: Diffuse texture */
    TSharedPtr<SImage2D> DiffuseTexture;

     /** @brief: Normal texture */
    TSharedPtr<SImage2D> NormalTexture;

     /** @brief: Specular texture - Stores AO, Metallic, and Roughness in the same textures */
    TSharedPtr<SImage2D> SpecularTexture;

     /** @brief: Emissive texture */
    TSharedPtr<SImage2D> EmissiveTexture;

     /** @brief: AO texture - Ambient Occlusion */
    TSharedPtr<SImage2D> AOTexture;

     /** @brief: Roughness texture*/
    TSharedPtr<SImage2D> RoughnessTexture;

     /** @brief: Metallic Texture*/
    TSharedPtr<SImage2D> MetallicTexture;

     /** @brief: Metallic Texture*/
    TSharedPtr<SImage2D> AlphaMaskTexture;

     /** @brief: Diffuse Parameter */
    CVector3 Diffuse;

     /** @brief: AO Parameter */
    float AO = 1.0f;

     /** @brief: Roughness Parameter */
    float Roughness = 1.0f;

     /** @brief: Metallic Parameter */
    float Metallic = 1.0f;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SceneData

struct ENGINE_API SSceneData
{

    void AddToScene(class CScene* Scene);

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

    TArray<SModelData>    Models;
    TArray<SMaterialData> Materials;

     /** @brief: A scale used to scale each actor when using add to scene */
    float Scale = 1.0f;
};
