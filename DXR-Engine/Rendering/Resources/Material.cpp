#include "Material.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/CommandList.h"

#define GET_SAFE_SRV(Texture) (Texture != nullptr) ? Texture->GetShaderResourceView() : nullptr

CMaterial::CMaterial(const SMaterialDesc& InProperties)
    : AlbedoMap()
    , NormalMap()
    , RoughnessMap()
    , MetallicMap()
    , AOMap()
    , HeightMap()
    , m_MaterialBuffer()
    , m_Properties(InProperties)
{
}

void CMaterial::Init()
{
    m_MaterialBuffer = CreateConstantBuffer<SMaterialDesc>(BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (m_MaterialBuffer)
    {
        m_MaterialBuffer->SetName("m_MaterialBuffer");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU       = ESamplerMode::Wrap;
    CreateInfo.AddressV       = ESamplerMode::Wrap;
    CreateInfo.AddressW       = ESamplerMode::Wrap;
    CreateInfo.ComparisonFunc = EComparisonFunc::Never;
    CreateInfo.Filter         = ESamplerFilter::Anistrotopic;
    CreateInfo.MaxAnisotropy  = 16;
    CreateInfo.MaxLOD         = FLT_MAX;
    CreateInfo.MinLOD         = -FLT_MAX;
    CreateInfo.MipLODBias     = 0.0f;

    m_Sampler = CreateSamplerState(CreateInfo);
}

void CMaterial::BuildBuffer(CommandList& CmdList)
{
    CmdList.TransitionBuffer(m_MaterialBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);
    CmdList.UpdateBuffer(m_MaterialBuffer.Get(), 0, sizeof(SMaterialDesc), &m_Properties);
    CmdList.TransitionBuffer(m_MaterialBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

    m_MaterialBufferIsDirty = false;
}

void CMaterial::SetAlbedo(const XMFLOAT3& Albedo)
{
    m_Properties.Albedo = Albedo;
    m_MaterialBufferIsDirty = true;
}

void CMaterial::SetAlbedo(float R, float G, float B)
{
    m_Properties.Albedo = XMFLOAT3(R, G, B);
    m_MaterialBufferIsDirty = true;
}

void CMaterial::SetMetallic(float Metallic)
{
    m_Properties.Metallic = Metallic;
    m_MaterialBufferIsDirty = true;
}

void CMaterial::SetRoughness(float Roughness)
{
    m_Properties.Roughness = Roughness;
    m_MaterialBufferIsDirty = true;
}

void CMaterial::SetAmbientOcclusion(float AO)
{
    m_Properties.AO = AO;
    m_MaterialBufferIsDirty = true;
}

void CMaterial::ForceForwardPass(bool ForceForwardRender)
{
    m_RenderInForwardPass = ForceForwardRender;
}

void CMaterial::EnableHeightMap(bool InEnableHeightMap)
{
    m_Properties.EnableHeight = (int32)InEnableHeightMap;
}

void CMaterial::EnableAlphaMask(bool InEnableAlphaMask)
{
    m_Properties.EnableMask = (int32)InEnableAlphaMask;
}

void CMaterial::SetDebugName(const std::string& DebugName)
{
    m_DebugName = DebugName;
}

ShaderResourceView* const* CMaterial::GetShaderResourceViews() const
{
    m_ShaderResourceViews[0] = GET_SAFE_SRV(AlbedoMap);
    m_ShaderResourceViews[1] = GET_SAFE_SRV(NormalMap);
    m_ShaderResourceViews[2] = GET_SAFE_SRV(RoughnessMap);
    m_ShaderResourceViews[3] = GET_SAFE_SRV(HeightMap);
    m_ShaderResourceViews[4] = GET_SAFE_SRV(MetallicMap);
    m_ShaderResourceViews[5] = GET_SAFE_SRV(AOMap);
    m_ShaderResourceViews[6] = GET_SAFE_SRV(AlphaMask);

    return m_ShaderResourceViews.Data();
}
