#pragma once
#include "Core.h"
#include "VertexFormat.h"
#include "TextureFormat.h"

#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"

struct SMeshData
{
    TArray<Vertex> Vertices;
    TArray<uint32> Indices;

    inline void Clear()
    {
        Vertices.Clear();
        Indices.Clear();
    }
};

struct SModelData
{
    /* Name of the mesh specified in the model-file */
    CString Name;

    /* Model mesh data */
    SMeshData Mesh;

    /* The Material index in the SSceneData Materials Array */
    int32 MaterialIndex = -1;
};

/* 2-D image for when loading materials */
struct SImage2D
{
    SImage2D() = default;

    SImage2D( const CString& InPath, uint16 InWidth, uint16 InHeight, EFormat InFormat )
        : Path( InPath )
        , Image()
        , Width( InWidth )
        , Height( InHeight )
        , Format( InFormat )
    {
    }

    /* Relative path to the image specified in the model-file */
    CString Path;

    /* Pointer to image data */
    TSharedPtr<uint8[]> Image;

    /* Size of the image */
    uint16 Width = 0;
    uint16 Height = 0;

    /* The format that the image was loaded as */
    EFormat Format = EFormat::Unknown;
};

/* Contains loaded data from a material */
struct SMaterialData
{
    /* Diffuse texture */
    TSharedPtr<SImage2D> DiffuseTexture;

    /* Normal texture */
    TSharedPtr<SImage2D> NormalTexture;

    /* Specular texture - Stores AO, Metallic, and Roughness in the same textures */
    TSharedPtr<SImage2D> SpecularTexture;

    /* Emissive texture */
    TSharedPtr<SImage2D> EmissiveTexture;

    /* AO texture - Ambient Occlusion */
    TSharedPtr<SImage2D> AOTexture;

    /* Roughness texture*/
    TSharedPtr<SImage2D> RoughnessTexture;

    /* Metallic Texture*/
    TSharedPtr<SImage2D> MetallicTexture;

    /* Metallic Texture*/
    TSharedPtr<SImage2D> AlphaMaskTexture;

    /* Diffuse Parameter */
    CVector3 Diffuse;

    /* AO Parameter */
    float AO = 1.0f;

    /* Roughness Parameter */
    float Roughness = 1.0f;

    /* Metallic Parameter */
    float Metallic = 1.0f;
};

struct SSceneData
{
    TArray<SModelData> Models;
    TArray<SMaterialData> Materials;

    void AddToScene( class Scene* Scene );

    FORCEINLINE bool HasData() const
    {
        return !Models.IsEmpty() && !Materials.IsEmpty();
    }
};
