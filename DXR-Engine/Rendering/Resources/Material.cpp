#include "Material.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/CommandList.h"

#define GET_SAFE_SRV(Texture) (Texture != nullptr) ? Texture->GetShaderResourceView() : nullptr

Material::Material(const MaterialProperties& InProperties)
    : AlbedoMap()
    , NormalMap()
    , RoughnessMap()
    , MetallicMap()
    , AOMap()
    , HeightMap()
    , MaterialBuffer()
    , Properties(InProperties)
{
}

void Material::Init()
{
    MaterialBuffer = CreateConstantBuffer<MaterialProperties>(BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (MaterialBuffer)
    {
        MaterialBuffer->SetName("MaterialBuffer");
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

    Sampler = CreateSamplerState(CreateInfo);
}

void Material::BuildBuffer(CommandList& CmdList)
{
    CmdList.TransitionBuffer(MaterialBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest);
    CmdList.UpdateBuffer(MaterialBuffer.Get(), 0, sizeof(MaterialProperties), &Properties);
    CmdList.TransitionBuffer(MaterialBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer);

    MaterialBufferIsDirty = false;
}

void Material::SetAlbedo(const XMFLOAT3& Albedo)
{
    Properties.Albedo = Albedo;
    MaterialBufferIsDirty = true;
}

void Material::SetAlbedo(Float R, Float G, Float B)
{
    Properties.Albedo = XMFLOAT3(R, G, B);
    MaterialBufferIsDirty = true;
}

void Material::SetMetallic(Float Metallic)
{
    Properties.Metallic = Metallic;
    MaterialBufferIsDirty = true;
}

void Material::SetRoughness(Float Roughness)
{
    Properties.Roughness = Roughness;
    MaterialBufferIsDirty = true;
}

void Material::SetAmbientOcclusion(Float AO)
{
    Properties.AO = AO;
    MaterialBufferIsDirty = true;
}

void Material::EnableHeightMap(Bool EnableHeightMap)
{
    if (EnableHeightMap)
    {
        Properties.EnableHeight = 1;
    }
    else
    {
        Properties.EnableHeight = 0;
    }
}

void Material::EnableEmissiveMap(Bool EnableEmissiveMap)
{
    if (EnableEmissiveMap)
    {
        Properties.EnableEmissive = 1;
    }
    else
    {
        Properties.EnableEmissive = 0;
    }
}

void Material::SetDebugName(const std::string& InDebugName)
{
    DebugName = InDebugName;
}

ShaderResourceView* const* Material::GetShaderResourceViews() const
{
    ShaderResourceViews[0] = GET_SAFE_SRV(AlbedoMap);
    ShaderResourceViews[1] = GET_SAFE_SRV(NormalMap);
    ShaderResourceViews[2] = GET_SAFE_SRV(MetallicMap);
    ShaderResourceViews[3] = GET_SAFE_SRV(HeightMap);
    ShaderResourceViews[4] = GET_SAFE_SRV(RoughnessMap);
    ShaderResourceViews[5] = GET_SAFE_SRV(AOMap);
    ShaderResourceViews[6] = GET_SAFE_SRV(AlphaMask);
    ShaderResourceViews[7] = GET_SAFE_SRV(EmissiveMap);

    return ShaderResourceViews.Data();
}
