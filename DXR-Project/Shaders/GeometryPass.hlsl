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
	float3 WorldPosition	: POSITION0;
	float3 Normal			: NORMAL0;
	float3 Tangent			: TANGENT0;
	float3 Bitangent		: BITANGENT0;
	float3 TangentViewPos	: TANGENTVIEWPOS0;
	float3 TangentPosition	: TANGENTPOSITION0;
	float2 TexCoord			: TEXCOORD0;
	float4 Position			: SV_Position;
};

VSOutput VSMain(VSInput Input)
{
	VSOutput Output;
    Output.Normal = normalize(mul(float4(Input.Normal, 0.0f), Transform));
	
    float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), Transform));
    Output.Tangent = normalize(Tangent - dot(Tangent, Output.Normal) * Output.Normal);
	
	Output.Bitangent = normalize(cross(Output.Tangent, Output.Normal));

	Output.TexCoord = Input.TexCoord;

	Output.WorldPosition	= mul(float4(Input.Position, 1.0f), Transform).xyz;
	Output.Position			= mul(float4(Output.WorldPosition, 1.0f), Camera.ViewProjection);
	
	float3x3 TBN = float3x3(Output.Tangent, Output.Bitangent, Output.Normal);
	TBN = transpose(TBN);
	Output.TangentViewPos	= mul(Camera.Position, TBN);
	Output.TangentPosition	= mul(Output.WorldPosition, TBN);
	
	return Output;
}

// PixelShader
struct PSInput
{
	float3 WorldPosition	: POSITION0;
	float3 Normal			: NORMAL0;
	float3 Tangent			: TANGENT0;
	float3 Bitangent		: BITANGENT0;
	float3 TangentViewPos	: TANGENTVIEWPOS0;
	float3 TangentPosition	: TANGENTPOSITION0;
	float2 TexCoord			: TEXCOORD0;
};

struct PSOutput
{
	float4 Albedo	: SV_Target0;
	float4 Normal	: SV_Target1;
	float4 Material : SV_Target2;
};

static const float HEIGHT_SCALE = 0.5f;

float2 ParallaxMapping(float2 TexCoords, float3 ViewDir)
{
	const float MinLayers = 4;
	const float MaxLayers = 16;

	float NumLayers		= lerp(MaxLayers, MinLayers, abs(dot(float3(0.0f, 0.0f, 1.0f), ViewDir)));
	float LayerDepth	= 1.0f / NumLayers;
	float CurrentLayerDepth = 0.0;
	float2 P = ViewDir.xy / (ViewDir.z * HEIGHT_SCALE);
	float2 DeltaTexCoords = P / NumLayers;

	float2 CurrentTexCoords = TexCoords;
	float CurrentDepthMapValue = HeightMap.Sample(MaterialSampler, CurrentTexCoords).r;

	while (CurrentLayerDepth < CurrentDepthMapValue)
	{
		CurrentTexCoords -= DeltaTexCoords;
		CurrentDepthMapValue = HeightMap.Sample(MaterialSampler, CurrentTexCoords).r;
		CurrentLayerDepth += LayerDepth;
	}

	float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

	float AfterDepth = CurrentDepthMapValue - CurrentLayerDepth;
	float BeforeDepth = HeightMap.Sample(MaterialSampler, CurrentTexCoords).r - CurrentLayerDepth + LayerDepth;

	float Weight = AfterDepth / (AfterDepth - BeforeDepth);
	float2 FinalTexCoords = PrevTexCoords * Weight + CurrentTexCoords * (1.0f - Weight);

	return FinalTexCoords;
}

PSOutput PSMain(PSInput Input)
{
	float3	ViewDir		= normalize(Input.TangentViewPos - Input.TangentPosition);
	float2	TexCoords	= ParallaxMapping(Input.TexCoord, ViewDir);
	if (TexCoords.x > 1.0f || TexCoords.y > 1.0f || TexCoords.x < 0.0f || TexCoords.y < 0.0f)
	{
		discard;
	}

    float3 Albedo			= AlbedoMap.Sample(MaterialSampler, TexCoords);
    float3 SampledNormal	= NormalMap.Sample(MaterialSampler, TexCoords);
    SampledNormal = UnpackNormal(SampledNormal);
	
    float3 Tangent		= normalize(Input.Tangent);
	float3 Bitangent	= normalize(Input.Bitangent);
    float3 Normal		= normalize(Input.Normal);
    float3 MappedNormal = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);
    MappedNormal = PackNormal(MappedNormal);
	
    float SampledAO				= AOMap.Sample(MaterialSampler, TexCoords).r * AO;
    float SampledRoughness		= RoughnessMap.Sample(MaterialSampler, TexCoords).r * Roughness;
    const float FinalRoughness	= min(max(SampledRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);
	
	PSOutput Output;
    Output.Albedo	= float4(Albedo, 1.0f);
    Output.Normal	= float4(MappedNormal, 1.0f);
    Output.Material = float4(FinalRoughness, Metallic, SampledAO, 1.0f);

	return Output;
}