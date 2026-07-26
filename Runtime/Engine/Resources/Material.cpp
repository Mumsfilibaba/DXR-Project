#include "Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Engine.h"
#include "Core/Math/Math.h"

FMaterial::FMaterial(const FMaterialInfo& InMaterialInfo)
    : AlbedoMap()
    , NormalMap()
    , MaterialMap()
    , HeightMap()
    , Name()
    , MaterialInfo(InMaterialInfo)
{
}

FMaterial::~FMaterial()
{
}

void FMaterial::Initialize()
{
    Sampler = FEngine::Get()->BaseMaterialSampler;
}

void FMaterial::FillMaterialData(FMaterialHLSL& OutData) const
{
    OutData.Albedo              = Vector3(MaterialInfo.Albedo.R, MaterialInfo.Albedo.G, MaterialInfo.Albedo.B);
    OutData.Metallic            = MaterialInfo.Metallic;
    OutData.Roughness           = MaterialInfo.Roughness;
    OutData.AmbientOcclusion    = MaterialInfo.AmbientOcclusion;
    OutData.ParallaxHeightScale = MaterialInfo.ParallaxHeightScale;
    OutData.ParallaxMinLayers   = MaterialInfo.ParallaxMinLayers;
    OutData.ParallaxMaxLayers   = MaterialInfo.ParallaxMaxLayers;
}

void FMaterial::SetAlbedo(const FFloatColor& Albedo)
{
    MaterialInfo.Albedo = Albedo;
}

void FMaterial::SetMetallic(float Metallic)
{
    MaterialInfo.Metallic = Metallic;
}

void FMaterial::SetRoughness(float Roughness)
{
    MaterialInfo.Roughness = Roughness;
}

void FMaterial::SetAmbientOcclusion(float AmbientOcclusion)
{
    MaterialInfo.AmbientOcclusion = AmbientOcclusion;
}

void FMaterial::SetMaterialFlags(EMaterialFlags InFlags, bool bUpdateOnly)
{
    if (bUpdateOnly)
    {
        MaterialInfo.MaterialFlags |= InFlags;
    }
    else
    {
        MaterialInfo.MaterialFlags = InFlags;
    }
}

void FMaterial::ForceForwardPass(bool bForceForwardRender)
{
    if (bForceForwardRender)
    {
        MaterialInfo.MaterialFlags |= EMaterialFlags::ForceForwardPass;
    }
}

void FMaterial::EnableHeightMap(bool bEnableHeightMap)
{
    if (bEnableHeightMap)
    {
        MaterialInfo.MaterialFlags |= EMaterialFlags::EnableHeight;
    }
}

void FMaterial::EnableAlphaMask(bool bEnableAlphaMask)
{
    if (bEnableAlphaMask)
    {
        MaterialInfo.MaterialFlags |= EMaterialFlags::EnableAlpha;
    }
}

void FMaterial::EnableDoubleSided(bool bIsDoubleSided)
{
    if (bIsDoubleSided)
    {
        MaterialInfo.MaterialFlags |= EMaterialFlags::DoubleSided;
    }
}

void FMaterial::SetParallaxHeightScale(float InParallaxHeightScale)
{
    MaterialInfo.ParallaxHeightScale = InParallaxHeightScale;
}

void FMaterial::SetParallaxLayers(float InParallaxMinLayers, float InParallaxMaxLayers)
{
    InParallaxMinLayers = Math::Max(InParallaxMinLayers, 1.0f);
    InParallaxMaxLayers = Math::Max(InParallaxMaxLayers, InParallaxMinLayers);

    MaterialInfo.ParallaxMinLayers = InParallaxMinLayers;
    MaterialInfo.ParallaxMaxLayers = InParallaxMaxLayers;
}

void FMaterial::SetName(const String& InName)
{
    Name = InName;
}
