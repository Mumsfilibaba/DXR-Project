#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "Engine/Assets/MeshData.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Texture.h"

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

struct FMaterialData
{
    FMaterialData()
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

struct FModelData
{
    FModelData()
        : Meshes()
        , Materials()
        , Scale(1.0f)
    {
    }

    TArray<FMeshData>     Meshes;
    TArray<FMaterialData> Materials;
    float                 Scale;
};
