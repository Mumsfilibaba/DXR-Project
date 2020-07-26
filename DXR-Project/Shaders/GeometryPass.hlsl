#include "PBRCommon.hlsli"

#define ENABLE_PARALLAX_MAPPING 1

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
	
	float3 Normal = normalize(mul(float4(Input.Normal, 0.0f), Transform).xyz);
	Output.Normal = Normal;
	
	float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), Transform).xyz);
	Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
	Output.Tangent = Tangent;
	
	float3 Bitangent = normalize(cross(Output.Tangent, Output.Normal));
	Output.Bitangent = Bitangent;

	Output.TexCoord = Input.TexCoord;

	float4 WorldPosition	= mul(float4(Input.Position, 1.0f), Transform);
	Output.WorldPosition	= WorldPosition.xyz;
	Output.Position			= mul(WorldPosition, Camera.ViewProjection);
	
	float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
	TBN = transpose(TBN);
	
	Output.TangentViewPos	= mul(Camera.Position, TBN);
	Output.TangentPosition	= mul(WorldPosition.xyz, TBN);
	
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

static const float HEIGHT_SCALE = 0.015f;

float SampleHeightMap(float2 TexCoords)
{
	return 1.0f - HeightMap.Sample(MaterialSampler, TexCoords).r;
}

float2 ParallaxMapping(float2 TexCoords, float3 ViewDir)
{
	const float MinLayers	= 8;
	const float MaxLayers	= 32;

	float NumLayers			= lerp(MaxLayers, MinLayers, abs(dot(float3(0.0f, 0.0f, 1.0f), ViewDir)));
	float LayerDepth		= 1.0f / NumLayers;
	
	float2 P				= ViewDir.xy / ViewDir.z * HEIGHT_SCALE;
	float2 DeltaTexCoords	= P / NumLayers;

	float2	CurrentTexCoords		= TexCoords;
	float	CurrentDepthMapValue	= SampleHeightMap(CurrentTexCoords);
	
	float CurrentLayerDepth	= 0.0f;
	while (CurrentLayerDepth < CurrentDepthMapValue)
	{
		CurrentTexCoords		-=	DeltaTexCoords;
		CurrentDepthMapValue	=	SampleHeightMap(CurrentTexCoords);
		CurrentLayerDepth		+=	LayerDepth;
	}

	float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

	float AfterDepth	= CurrentDepthMapValue - CurrentLayerDepth;
	float BeforeDepth	= SampleHeightMap(PrevTexCoords) - CurrentLayerDepth + LayerDepth;

	float	Weight			= AfterDepth / (AfterDepth - BeforeDepth);
	float2	FinalTexCoords	= PrevTexCoords * Weight + CurrentTexCoords * (1.0f - Weight);

	return FinalTexCoords;
}

PSOutput PSMain(PSInput Input)
{
	float3	ViewDir		= normalize(Input.TangentViewPos - Input.TangentPosition);
	float2	TexCoords	= Input.TexCoord;
	TexCoords.y = 1.0f - TexCoords.y;
	
#if ENABLE_PARALLAX_MAPPING
	TexCoords = ParallaxMapping(TexCoords, ViewDir);
	//if (TexCoords.x > 1.0f || TexCoords.y > 1.0f || TexCoords.x < 0.0f || TexCoords.y < 0.0f)
	//{
	//    discard;
	//}
#endif

	float3 Albedo			= AlbedoMap.Sample(MaterialSampler, TexCoords).rgb;
	float3 SampledNormal	= NormalMap.Sample(MaterialSampler, TexCoords).rgb;
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