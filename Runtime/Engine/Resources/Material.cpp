#include "Material.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHICommandList.h"

#include "Engine/Engine.h"

#define GET_SAFE_SRV(Texture) (Texture != nullptr) ? Texture->GetShaderResourceView() : nullptr

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
{
}

void CMaterial::Init()
{
    MaterialBuffer = RHICreateConstantBuffer<SMaterialDesc>(BufferFlag_Default, ERHIResourceAccess::VertexAndConstantBuffer, nullptr);
    if (MaterialBuffer)
    {
        MaterialBuffer->SetName("MaterialBuffer");
    }

    Sampler = GEngine->BaseMaterialSampler;
}

void CMaterial::BuildBuffer(CRHICommandList& CmdList)
{
    CmdList.TransitionBuffer(MaterialBuffer.Get(), ERHIResourceAccess::VertexAndConstantBuffer, ERHIResourceAccess::CopyDest);
    CmdList.UpdateBuffer(MaterialBuffer.Get(), 0, sizeof(SMaterialDesc), &Properties);
    CmdList.TransitionBuffer(MaterialBuffer.Get(), ERHIResourceAccess::CopyDest, ERHIResourceAccess::VertexAndConstantBuffer);

    bMaterialBufferIsDirty = false;
}

void CMaterial::SetAlbedo(const CVector3& Albedo)
{
    Properties.Albedo = Albedo;
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetAlbedo(float r, float g, float b)
{
    Properties.Albedo = CVector3(r, g, b);
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetMetallic(float Metallic)
{
    Properties.Metallic = Metallic;
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetRoughness(float Roughness)
{
    Properties.Roughness = Roughness;
    bMaterialBufferIsDirty = true;
}

void CMaterial::SetAmbientOcclusion(float AO)
{
    Properties.AO = AO;
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
    ShaderResourceViews[0] = GET_SAFE_SRV(AlbedoMap);
    ShaderResourceViews[1] = GET_SAFE_SRV(NormalMap);
    ShaderResourceViews[2] = GET_SAFE_SRV(RoughnessMap);
    ShaderResourceViews[3] = GET_SAFE_SRV(HeightMap);
    ShaderResourceViews[4] = GET_SAFE_SRV(MetallicMap);
    ShaderResourceViews[5] = GET_SAFE_SRV(AOMap);
    ShaderResourceViews[6] = GET_SAFE_SRV(AlphaMask);

    return ShaderResourceViews.Data();
}
