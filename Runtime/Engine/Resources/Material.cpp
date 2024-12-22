#include "Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Engine.h"

FMaterial::FMaterial(const FMaterialInfo& InMaterialInfo)
    : AlbedoMap()
    , NormalMap()
    , RoughnessMap()
    , HeightMap()
    , AOMap()
    , MetallicMap()
    , Name()
    , MaterialData()
    , MaterialInfo(InMaterialInfo)
    , bMaterialBufferIsDirty(true)
    , MaterialBuffer()
{
}

FMaterial::~FMaterial()
{
}

void FMaterial::Initialize()
{
    FRHIBufferInfo BufferInfo(sizeof(FMaterialHLSL), sizeof(FMaterialHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
    MaterialBuffer = RHICreateBuffer(BufferInfo, EResourceAccess::ConstantBuffer, nullptr);
    if (MaterialBuffer)
    {
        MaterialBuffer->SetDebugName("MaterialBuffer");
    }

    Sampler = GEngine->BaseMaterialSampler;
}

void FMaterial::BuildBuffer(FRHICommandList& CommandList)
{
    MaterialData.Albedo           = FVector3(MaterialInfo.Albedo.R, MaterialInfo.Albedo.G, MaterialInfo.Albedo.B);
    MaterialData.Metallic         = MaterialInfo.Metallic;
    MaterialData.Roughness        = MaterialInfo.Roughness;
    MaterialData.AmbientOcclusion = MaterialInfo.AmbientOcclusion;

    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(MaterialBuffer.Get(), FBufferRegion(0, sizeof(FMaterialHLSL)), &MaterialData);
    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    bMaterialBufferIsDirty = false;
}

void FMaterial::SetAlbedo(const FFloatColor& Albedo)
{
    MaterialInfo.Albedo    = Albedo;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetMetallic(float Metallic)
{
    MaterialInfo.Metallic  = Metallic;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetRoughness(float Roughness)
{
    MaterialInfo.Roughness = Roughness;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetAmbientOcclusion(float AmbientOcclusion)
{
    MaterialInfo.AmbientOcclusion = AmbientOcclusion;
    bMaterialBufferIsDirty        = true;
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

void FMaterial::SetName(const FString& InName)
{
    Name = InName;
}
