#include "Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Engine.h"

FMaterial::FMaterial(const FMaterialDesc& InProperties)
    : AlbedoMap()
    , NormalMap()
    , RoughnessMap()
    , HeightMap()
    , AOMap()
    , MetallicMap()
    , Properties(InProperties)
    , MaterialBuffer()
{ }

void FMaterial::Initialize()
{
    FRHIBufferDesc Desc(
        sizeof(FMaterialDesc),
        sizeof(FMaterialDesc),
        EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);

    MaterialBuffer = RHICreateBuffer(Desc, EResourceAccess::VertexAndConstantBuffer, nullptr);
    if (MaterialBuffer)
    {
        MaterialBuffer->SetName("MaterialBuffer");
    }

    Sampler = GEngine->BaseMaterialSampler;
}

void FMaterial::BuildBuffer(FRHICommandList& CommandList)
{
    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
    CommandList.UpdateBuffer(MaterialBuffer.Get(), FBufferRegion(0, sizeof(FMaterialDesc)), &Properties);
    CommandList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

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

void FMaterial::SetAmbientOcclusion(float AO)
{
    Properties.AO          = AO;
    bMaterialBufferIsDirty = true;
}

void FMaterial::ForceForwardPass(bool bForceForwardRender)
{
    bRenderInForwardPass = bForceForwardRender;
}

void FMaterial::EnableHeightMap(bool bInEnableHeightMap)
{
    Properties.EnableHeight = bInEnableHeightMap ? 1 : 0;
}

void FMaterial::EnableAlphaMask(bool bInEnableAlphaMask)
{
    Properties.EnableMask = bInEnableAlphaMask ? 1 : 0;
}

void FMaterial::SetDebugName(const FString& InDebugName)
{
    DebugName = InDebugName;
}

FRHIShaderResourceView* const* FMaterial::GetShaderResourceViews() const
{
    ShaderResourceViews[0] = SafeGetDefaultSRV(AlbedoMap);
    ShaderResourceViews[1] = SafeGetDefaultSRV(NormalMap);
    ShaderResourceViews[2] = SafeGetDefaultSRV(RoughnessMap);
    ShaderResourceViews[3] = SafeGetDefaultSRV(HeightMap);
    ShaderResourceViews[4] = SafeGetDefaultSRV(MetallicMap);
    ShaderResourceViews[5] = SafeGetDefaultSRV(AOMap);
    ShaderResourceViews[6] = SafeGetDefaultSRV(AlphaMask);

    return ShaderResourceViews.Data();
}
