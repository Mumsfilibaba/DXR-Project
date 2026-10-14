#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "AssetCore/ImportScene.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Texture.h"

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

    FString                Name;
    TSharedPtr<FTexture2D> Textures[EMaterialTexture::Count];
    FMaterialSourceRoute   Routes[EMaterialScalar::Count];
    FVector3               Diffuse;
    float                  AmbientFactor;
    float                  Roughness;
    float                  Metallic;
    EMaterialFlags         MaterialFlags;
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
