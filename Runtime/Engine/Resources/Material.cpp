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
    , MaterialData()
    , MaterialInfo(InMaterialInfo)
    , MaterialBuffer()
    , bMaterialBufferIsDirty(true)
    , DebugName()
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
    MaterialData.Albedo           = MaterialInfo.Albedo;
    MaterialData.Metallic         = MaterialInfo.Metallic;
    MaterialData.Roughness        = MaterialInfo.Roughness;
    MaterialData.AmbientOcclusion = MaterialInfo.AmbientOcclusion;

    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(MaterialBuffer.Get(), FBufferRegion(0, sizeof(FMaterialHLSL)), &MaterialData);
    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);
    bMaterialBufferIsDirty = false;
}

void FMaterial::SetAlbedo(const FVector3& Albedo)
{
    MaterialInfo.Albedo      = Albedo;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetAlbedo(float r, float g, float b)
{
    MaterialInfo.Albedo      = FVector3(r, g, b);
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetMetallic(float Metallic)
{
    MaterialInfo.Metallic    = Metallic;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetRoughness(float Roughness)
{
    MaterialInfo.Roughness   = Roughness;
    bMaterialBufferIsDirty = true;
}

void FMaterial::SetAmbientOcclusion(float AmbientOcclusion)
{
    MaterialInfo.AmbientOcclusion = AmbientOcclusion;
    bMaterialBufferIsDirty      = true;
}

void FMaterial::ForceForwardPass(bool bForceForwardRender)
{
    if (bForceForwardRender)
        MaterialInfo.MaterialFlags |= MaterialFlag_ForceForwardPass;
}

void FMaterial::EnableHeightMap(bool bEnableHeightMap)
{
    if (bEnableHeightMap)
        MaterialInfo.MaterialFlags |= MaterialFlag_EnableHeight;
}

void FMaterial::EnableAlphaMask(bool bEnableAlphaMask)
{
    if (bEnableAlphaMask)
        MaterialInfo.MaterialFlags |= MaterialFlag_EnableAlpha;
}

void FMaterial::EnableDoubleSided(bool bIsDoubleSided)
{
    if (bIsDoubleSided)
        MaterialInfo.MaterialFlags |= MaterialFlag_DoubleSided;
}

void FMaterial::SetDebugName(const FString& InDebugName)
{
    DebugName = InDebugName;
}
