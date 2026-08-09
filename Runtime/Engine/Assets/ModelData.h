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

struct FMaterialSourceRoute
{
    FMaterialSourceRoute() = default;

    FMaterialSourceRoute(EMaterialTexture::Type InTexture, ETextureChannel InChannel, bool bInInvert = false)
        : Texture(InTexture)
        , Channel(InChannel)
        , bInvert(bInInvert)
    {
    }

    bool IsRouted() const
    {
        return Texture < EMaterialTexture::Count;
    }

    EMaterialTexture::Type Texture = EMaterialTexture::Count;
    ETextureChannel        Channel = ETextureChannel::R;
    bool                   bInvert = false;
};

struct FMaterialData
{
    FMaterialData()
        : Name()
        , Textures()
        , Routes()
        , Diffuse()
        , AmbientFactor(1.0f)
        , Roughness(1.0f)
        , Metallic()
        , MaterialFlags(EMaterialFlags::None)
    {
        Routes[EMaterialScalar::Opacity] = FMaterialSourceRoute(EMaterialTexture::Diffuse, ETextureChannel::A);
    }

    String               Name;
    FTexture2DRef        Textures[EMaterialTexture::Count];
    FMaterialSourceRoute Routes[EMaterialScalar::Count];
    Vector3              Diffuse;
    float                AmbientFactor;
    float                Roughness;
    float                Metallic;
    EMaterialFlags       MaterialFlags;
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
