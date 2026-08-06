#include "Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Engine.h"
#include "RendererCore/VertexDeclaration.h"
#include "Core/Math/Math.h"

EMaterialFlags FMaterial::GetSupportedMaterialFlags(const FVertexDeclaration& Declaration)
{
    EMaterialFlags Supported = EMaterialFlags::DoubleSided | EMaterialFlags::ForceForwardPass;

    const bool bHasTangentBasis = Declaration.HasAttributes(EVertexAttributeFlags::TangentBasis);
    const bool bHasTexCoord     = Declaration.HasAttributes(EVertexAttributeFlags::TexCoord0);

    SetEnumFlag(Supported, EMaterialFlags::EnableAlpha, bHasTexCoord);
    SetEnumFlag(Supported, EMaterialFlags::EnableNormalMapping, bHasTangentBasis && bHasTexCoord);
    SetEnumFlag(Supported, EMaterialFlags::NormalMapPositiveY, bHasTangentBasis && bHasTexCoord);
    SetEnumFlag(Supported, EMaterialFlags::EnableHeight, bHasTangentBasis && bHasTexCoord);
    SetEnumFlag(Supported, EMaterialFlags::EnableParallaxClipping, bHasTangentBasis && bHasTexCoord);

    return Supported;
}

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
    SetEnumFlag(MaterialInfo.MaterialFlags, EMaterialFlags::ForceForwardPass, bForceForwardRender);
}

void FMaterial::EnableHeightMap(bool bEnableHeightMap)
{
    SetEnumFlag(MaterialInfo.MaterialFlags, EMaterialFlags::EnableHeight, bEnableHeightMap);
}

void FMaterial::EnableAlphaMask(bool bEnableAlphaMask)
{
    SetEnumFlag(MaterialInfo.MaterialFlags, EMaterialFlags::EnableAlpha, bEnableAlphaMask);
}

void FMaterial::EnableNormalMapping(bool bEnableNormalMapping)
{
    SetEnumFlag(MaterialInfo.MaterialFlags, EMaterialFlags::EnableNormalMapping, bEnableNormalMapping);
}

void FMaterial::SetNormalMapPositiveY(bool bPositiveY)
{
    SetEnumFlag(MaterialInfo.MaterialFlags, EMaterialFlags::NormalMapPositiveY, bPositiveY);
}

void FMaterial::EnableDoubleSided(bool bIsDoubleSided)
{
    SetEnumFlag(MaterialInfo.MaterialFlags, EMaterialFlags::DoubleSided, bIsDoubleSided);
}

void FMaterial::EnableParallaxClipping(bool bEnableParallaxClipping)
{
    SetEnumFlag(MaterialInfo.MaterialFlags, EMaterialFlags::EnableParallaxClipping, bEnableParallaxClipping);
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
