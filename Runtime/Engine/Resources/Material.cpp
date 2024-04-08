#include "Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Engine.h"

FMaterial::FMaterial(const FMaterialCreateInfo& InProperties)
    : AlbedoMap()
    , NormalMap()
    , RoughnessMap()
    , HeightMap()
    , AOMap()
    , MetallicMap()
    , MaterialData()
    , Properties(InProperties)
    , MaterialBuffer()
    , bMaterialBufferIsDirty(true)
    , DebugName()
{
}

void FMaterial::Initialize()
{
    FRHIBufferDesc Desc(sizeof(FMaterialHLSL), sizeof(FMaterialHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
    MaterialBuffer = RHICreateBuffer(Desc, EResourceAccess::ConstantBuffer, nullptr);
    if (MaterialBuffer)
    {
        MaterialBuffer->SetDebugName("MaterialBuffer");
    }

    Sampler = GEngine->BaseMaterialSampler;
}

void FMaterial::BuildBuffer(FRHICommandList& CommandList)
{
    MaterialData.Albedo           = Properties.Albedo;
    MaterialData.Metallic         = Properties.Metallic;
    MaterialData.Roughness        = Properties.Roughness;
    MaterialData.AmbientOcclusion = Properties.AmbientOcclusion;

    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(MaterialBuffer.Get(), FBufferRegion(0, sizeof(FMaterialHLSL)), &MaterialData);
    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    bMaterialBufferIsDirty = false;
}

void FMaterial::SetAlbedo(const FVector3& Albedo)
{
    Properties.Albedo      = Albedo;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetAlbedo(float r, float g, float b)
{
    Properties.Albedo      = FVector3(r, g, b);
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetMetallic(float Metallic)
{
    Properties.Metallic    = Metallic;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetRoughness(float Roughness)
{
    Properties.Roughness   = Roughness;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetAmbientOcclusion(float AmbientOcclusion)
{
    Properties.AmbientOcclusion = AmbientOcclusion;
    bMaterialBufferIsDirty      = true;
}

void FMaterial::ForceForwardPass(bool bForceForwardRender)
{
    if (bForceForwardRender)
        Properties.MaterialFlags |= MaterialFlag_ForceForwardPass;
}

void FMaterial::EnableHeightMap(bool bEnableHeightMap)
{
    if (bEnableHeightMap)
        Properties.MaterialFlags |= MaterialFlag_EnableHeight;
}

void FMaterial::EnableAlphaMask(bool bEnableAlphaMask)
{
    if (bEnableAlphaMask)
        Properties.MaterialFlags |= MaterialFlag_EnableAlpha;
}

void FMaterial::EnableDoubleSided(bool bIsDoubleSided)
{
    if (bIsDoubleSided)
        Properties.MaterialFlags |= MaterialFlag_DoubleSided;
}

void FMaterial::SetDebugName(const FString& InDebugName)
{
    DebugName = InDebugName;
}
