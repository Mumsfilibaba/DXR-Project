#pragma once
#include "Core.h"
#include "VertexFormat.h"

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
    String Name;

    /* Model mesh data */
    SMeshData Mesh;

    /* The Material index in the SSceneData Materials Array */
    int32 MaterialIndex = -1;
};

enum class EImageFormat : uint8
{
    /* Unknown */
    None = 0,
    
    /* RGBA - Uint8 */
    R8G8B8A8_Unorm = 1,

    /* RGBA - Float32 */
    R32G32B32A32_Float = 2
};

/* 2-D image for when loading materials */
struct SImage2D
{
    /* Path to the image specified in the model-file */
    String Path;

    /* Pointer to image data */
    TSharedPtr<uint8[]> Image;

    /* Size of the image */
    uint16 Width;
    uint16 Height;

    /* The format that the image was loaded as */
    EImageFormat Format = EImageFormat::None;
};

/* Contains loaded data from a material */
struct SMaterialData
{
    /* Diffuse texture */
    TSharedPtr<SImage2D> DiffuseTexture;
    
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
};