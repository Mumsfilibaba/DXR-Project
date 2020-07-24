#include "PBRCommon.hlsli"

// Scene and Output
cbuffer TransformBuffer : register(b0)
{
	float4x4	Transform;
	float		Roughness;
	float		Metallic;
	float		AO;
};

// PerFrame DescriptorTable
ConstantBuffer<Camera> Camera : register(b1, space0);

// PerObject DescriptorTable
Texture2D<float4> AlbedoMap		: register(t0, space0);
Texture2D<float4> NormalMap		: register(t1, space0);
Texture2D<float4> RoughnessMap	: register(t2, space0);
Texture2D<float4> HeightMap		: register(t3, space0);
Texture2D<float4> AOMap			: register(t4, space0);

SamplerState MaterialSampler : register(s0, space0);

// VertexShader
struct VSInput
{
	float3 Position : POSITION0;
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
	float4 Position : SV_Position;
};

VSOutput VSMain(VSInput Input)
{
	VSOutput Output;
    Output.Normal = normalize(mul(float4(Input.Normal, 0.0f), Transform));
	
    float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), Transform));
    Output.Tangent = normalize(Tangent - dot(Tangent, Output.Normal) * Output.Normal);
	
	Output.TexCoord = Input.TexCoord;
	Output.Position = mul(mul(float4(Input.Position, 1.0f), Transform), Camera.ViewProjection);
	return Output;
}

// PixelShader
struct PSInput
{
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
};

struct PSOutput
{
	float4 Albedo	: SV_Target0;
	float4 Normal	: SV_Target1;
	float4 Material : SV_Target2;
};

PSOutput PSMain(PSInput Input)
{
    float3 Albedo			= AlbedoMap.Sample(MaterialSampler, Input.TexCoord);
    float3 SampledNormal	= NormalMap.Sample(MaterialSampler, Input.TexCoord);
    SampledNormal = UnpackNormal(SampledNormal);
	
    float3 Normal		= normalize(Input.Normal);
    float3 Tangent		= normalize(Input.Tangent);
    float3 MappedNormal = ApplyNormalMapping(SampledNormal, Normal, Tangent);
    MappedNormal = PackNormal(MappedNormal);
	
    float SampledAO				= AOMap.Sample(MaterialSampler, Input.TexCoord).r * AO;
    float SampledHeight			= HeightMap.Sample(MaterialSampler, Input.TexCoord).r;
    float SampledRoughness		= RoughnessMap.Sample(MaterialSampler, Input.TexCoord).r * Roughness;
    const float FinalRoughness	= min(max(SampledRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);
	
	PSOutput Output;
    Output.Albedo	= float4(Albedo, 1.0f);
    Output.Normal	= float4(MappedNormal, 1.0f);
    Output.Material = float4(FinalRoughness, Metallic, SampledAO, SampledHeight);
	return Output;
}