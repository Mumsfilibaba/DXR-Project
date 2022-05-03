#include "Material.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHICommandList.h"

#include "Engine/Engine.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CMaterial

CMaterial::CMaterial(const SMaterialDesc& InProperties)
    : AlbedoMap()
    , NormalMap()
    , RoughnessMap()
    , HeightMap()
    , AOMap()
    , MetallicMap()
    , Properties(InProperties)
    , MaterialBuffer()
{ }

void CMaterial::Init()
{
    CRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, sizeof(SMaterialDesc));

    MaterialBuffer = RHICreateConstantBuffer(Initializer);
    if (MaterialBuffer)
    {
        MaterialBuffer->SetName("MaterialBuffer");
    }

    Sampler = GEngine->BaseMaterialSampler;
}

void CMaterial::BuildBuffer(CRHICommandList& CmdList)
{
    CmdList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
    CmdList.UpdateBuffer(MaterialBuffer.Get(), 0, sizeof(SMaterialDesc), &Properties);
    CmdList.TransitionBuffer(MaterialBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

    bMaterialBufferIsDirty = false;
}

void CMaterial::SetAlbedo(const CVector3& Albedo)
{
    Properties.Albedo      = Albedo;
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetAlbedo(float r, float g, float b)
{
    Properties.Albedo      = CVector3(r, g, b);
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetMetallic(float Metallic)
{
    Properties.Metallic    = Metallic;
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetRoughness(float Roughness)
{
    Properties.Roughness   = Roughness;
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetAmbientOcclusion(float AO)
{
    Properties.AO          = AO;
    bMaterialBufferIsDirty = true;
}

void CMaterial::ForceForwardPass(bool bForceForwardRender)
{
    bRenderInForwardPass = bForceForwardRender;
}

void CMaterial::EnableHeightMap(bool bInEnableHeightMap)
{
    Properties.EnableHeight = (int32)bInEnableHeightMap;
}

void CMaterial::EnableAlphaMask(bool bInEnableAlphaMask)
{
    Properties.EnableMask = (int32)bInEnableAlphaMask;
}

void CMaterial::SetDebugName(const String& InDebugName)
{
    DebugName = InDebugName;
}

CRHIShaderResourceView* const* CMaterial::GetShaderResourceViews() const
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
