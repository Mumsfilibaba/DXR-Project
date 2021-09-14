#include "Material.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/CommandList.h"

#define GET_SAFE_SRV(Texture) (Texture != nullptr) ? Texture->GetShaderResourceView() : nullptr

CMaterial::CMaterial( const SMaterialDesc& InProperties )
    : AlbedoMap()
    , NormalMap()
    , RoughnessMap()
	, HeightMap()
	, AOMap()
    , MetallicMap()
	, Properties( InProperties )
    , MaterialBuffer()
{
}

void CMaterial::Init()
{
    MaterialBuffer = CreateConstantBuffer<SMaterialDesc>( BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
    if ( MaterialBuffer )
    {
        MaterialBuffer->SetName( "MaterialBuffer" );
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Wrap;
    CreateInfo.AddressV = ESamplerMode::Wrap;
    CreateInfo.AddressW = ESamplerMode::Wrap;
    CreateInfo.ComparisonFunc = EComparisonFunc::Never;
    CreateInfo.Filter = ESamplerFilter::Anistrotopic;
    CreateInfo.MaxAnisotropy = 16;
    CreateInfo.MaxLOD = FLT_MAX;
    CreateInfo.MinLOD = -FLT_MAX;
    CreateInfo.MipLODBias = 0.0f;

    Sampler = CreateSamplerState( CreateInfo );
}

void CMaterial::BuildBuffer( CommandList& CmdList )
{
    CmdList.TransitionBuffer( MaterialBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );
    CmdList.UpdateBuffer( MaterialBuffer.Get(), 0, sizeof( SMaterialDesc ), &Properties );
    CmdList.TransitionBuffer( MaterialBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );

    MaterialBufferIsDirty = false;
}

void CMaterial::SetAlbedo( const CVector3& Albedo )
{
    Properties.Albedo = Albedo;
    MaterialBufferIsDirty = true;
}

void CMaterial::SetAlbedo( float r, float g, float b )
{
    Properties.Albedo = CVector3( r, g, b );
    MaterialBufferIsDirty = true;
}

void CMaterial::SetMetallic( float Metallic )
{
    Properties.Metallic = Metallic;
    MaterialBufferIsDirty = true;
}

void CMaterial::SetRoughness( float Roughness )
{
    Properties.Roughness = Roughness;
    MaterialBufferIsDirty = true;
}

void CMaterial::SetAmbientOcclusion( float AO )
{
    Properties.AO = AO;
    MaterialBufferIsDirty = true;
}

void CMaterial::ForceForwardPass( bool ForceForwardRender )
{
    RenderInForwardPass = ForceForwardRender;
}

void CMaterial::EnableHeightMap( bool InEnableHeightMap )
{
    Properties.EnableHeight = (int32)InEnableHeightMap;
}

void CMaterial::EnableAlphaMask( bool InEnableAlphaMask )
{
    Properties.EnableMask = (int32)InEnableAlphaMask;
}

void CMaterial::SetDebugName( const std::string& InDebugName )
{
    DebugName = InDebugName;
}

ShaderResourceView* const* CMaterial::GetShaderResourceViews() const
{
    ShaderResourceViews[0] = GET_SAFE_SRV( AlbedoMap );
    ShaderResourceViews[1] = GET_SAFE_SRV( NormalMap );
    ShaderResourceViews[2] = GET_SAFE_SRV( RoughnessMap );
    ShaderResourceViews[3] = GET_SAFE_SRV( HeightMap );
    ShaderResourceViews[4] = GET_SAFE_SRV( MetallicMap );
    ShaderResourceViews[5] = GET_SAFE_SRV( AOMap );
    ShaderResourceViews[6] = GET_SAFE_SRV( AlphaMask );

    return ShaderResourceViews.Data();
}
