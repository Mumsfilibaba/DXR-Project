#include "Material.h"
#include "Renderer.h"

#include "RenderLayer/CommandList.h"

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
    MaterialBuffer = RenderLayer::CreateConstantBuffer<MaterialProperties>(
        nullptr, 
        BufferUsage_Default,
        EResourceState::ResourceState_Common);

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU       = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressV       = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressW       = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.ComparisonFunc = EComparisonFunc::ComparisonFunc_Never;
    CreateInfo.Filter         = ESamplerFilter::SamplerFilter_Anistrotopic;
    CreateInfo.MaxAnisotropy  = 16;
    CreateInfo.MaxLOD         = FLT_MAX;
    CreateInfo.MinLOD         = -FLT_MAX;
    CreateInfo.MipLODBias     = 0.0f;

    Sampler = RenderLayer::CreateSamplerState(CreateInfo);
}

void Material::BuildBuffer(CommandList& CmdList)
{
    CmdList.TransitionBuffer(
        MaterialBuffer.Get(),
        EResourceState::ResourceState_VertexAndConstantBuffer,
        EResourceState::ResourceState_CopyDest);

    CmdList.UpdateBuffer(
        MaterialBuffer.Get(),
        0, sizeof(MaterialProperties),
        &Properties);

    CmdList.TransitionBuffer(
        MaterialBuffer.Get(),
        EResourceState::ResourceState_CopyDest,
        EResourceState::ResourceState_VertexAndConstantBuffer);

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

void Material::SetDebugName(const std::string& InDebugName)
{
    DebugName = InDebugName;
}

ShaderResourceView* const* Material::GetShaderResourceViews() const
{
    ShaderResourceViews[0] = AlbedoMap.View.Get();
    ShaderResourceViews[1] = NormalMap.View.Get();
    ShaderResourceViews[2] = RoughnessMap.View.Get();
    ShaderResourceViews[3] = HeightMap.View.Get();
    ShaderResourceViews[4] = MetallicMap.View.Get();
    ShaderResourceViews[5] = AOMap.View.Get();
    ShaderResourceViews[6] = AlphaMask.View.Get();

    return ShaderResourceViews.Data();
}
