#include "PBRCommon.hlsli"

#if ENABLE_PARALLAX_MAPPING
#define PARALLAX_MAPPING_ENABLED
#endif

#if ENABLE_NORMAL_MAPPING
#define NORMAL_MAPPING_ENABLED
#endif

// Per Frame Buffers
ConstantBuffer<Camera>				CameraBuffer		: register(b0, space1);
ConstantBuffer<PointLight>			PointLightBuffer	: register(b1, space1);
ConstantBuffer<DirectionalLight>	DirLightBuffer		: register(b2, space1);

// Per Frame Samplers
SamplerState MaterialSampler	: register(s0, space1);
SamplerState LUTSampler			: register(s1, space1);
SamplerState IrradianceSampler	: register(s2, space1);

SamplerComparisonState ShadowMapSampler0	: register(s3, space1);
SamplerState ShadowMapSampler1				: register(s4, space1);

// Per Frame Textures
TextureCube<float4> IrradianceMap			: register(t0, space1);
TextureCube<float4> SpecularIrradianceMap	: register(t1, space1);
Texture2D<float4>	IntegrationLUT			: register(t2, space1);
Texture2D<float2>	DirLightShadowMaps		: register(t3, space1);
TextureCube<float>	PointLightShadowMaps	: register(t4, space1);

// Per Object Buffers
ConstantBuffer<Transform>	TransformBuffer	: register(b0, space0);
ConstantBuffer<Material>	MaterialBuffer	: register(b1, space0);

// Per Object Textures
Texture2D<float4> AlbedoMap : register(t0, space0);
#ifdef NORMAL_MAPPING_ENABLED
Texture2D<float4> NormalMap : register(t1, space0);
#endif
Texture2D<float4> RoughnessMap : register(t2, space0);
#ifdef PARALLAX_MAPPING_ENABLED
Texture2D<float4> HeightMap : register(t3, space0);
#endif
Texture2D<float> MetallicMap	: register(t4, space0);
Texture2D<float> AOMap			: register(t5, space0);
Texture2D<float> AlphaMask		: register(t6, space0);

// Light Calculations
float3 CalcRadiance(float3 F0, float3 InNormal, float3 InViewDir, float3 InLightDir, float3 InRadiance, float3 InAlbedo, float InRoughness, float InMetallic)
{
	float3 HalfVector = normalize(InViewDir + InLightDir);
	
	// Cook-Torrance BRDF
	float NDF	= DistributionGGX(InNormal, HalfVector, InRoughness);
	float G		= GeometrySmith(InNormal, InViewDir, InLightDir, InRoughness);
	float3 F	= FresnelSchlick(saturate(dot(HalfVector, InViewDir)), F0);
	
	float	DotNV		= max(dot(InNormal, InViewDir), MIN_VALUE);
	float3	Nominator	= NDF * G * F;
	float	Denominator	= 4.0f * DotNV * max(dot(InNormal, InLightDir), 0.0f);
	float3	Specular	= Nominator / max(Denominator, MIN_VALUE);
		
	// Ks is equal to Fresnel
	float3 Ks = F;
	float3 Kd = 1.0f - Ks;
	Kd *= 1.0f - InMetallic;

	// Scale light by DotNL
	float DotNL = max(dot(InNormal, InLightDir), 0.0f);

	// Add to outgoing radiance Lo
	return (Kd * InAlbedo / PI + Specular) * InRadiance * DotNL;
}

// Shadow Mapping
#define ENABLE_POISSON_FILTERING	0
#define ENABLE_VSM					0
#define POISSON_SAMPLES				2

#if ENABLE_POISSON_FILTERING
#define TOTAL_POISSON_SAMPLES 16

static const float2 PoissonDisk[16] =
{
	float2(-0.94201624,  -0.39906216),
	float2( 0.94558609,  -0.76890725),
	float2(-0.094184101, -0.92938870),
	float2( 0.34495938,   0.29387760),
	float2(-0.91588581,   0.45771432),
	float2(-0.81544232,  -0.87912464),
	float2(-0.38277543,   0.27676845),
	float2( 0.97484398,   0.75648379),
	float2( 0.44323325,  -0.97511554),
	float2( 0.53742981,  -0.47373420),
	float2(-0.26496911,  -0.41893023),
	float2( 0.79197514,   0.19090188),
	float2(-0.24188840,   0.99706507),
	float2(-0.81409955,   0.91437590),
	float2( 0.19984126,   0.78641367),
	float2( 0.14383161,  -0.14100790)
};

float CalculatePoissonShadow(float3 WorldPosition, float2 TexCoord, float CompareDepth, float FarPlane)
{
	float Shadow = 0.0f;
	const float DiskRadius = (0.4f + (CompareDepth)) / FarPlane;
	
	[unroll]
	for (int i = 0; i < POISSON_SAMPLES; i++)
	{
		int Index = int(float(TOTAL_POISSON_SAMPLES) * Random(floor(WorldPosition.xyz * 1000.0f), i)) % TOTAL_POISSON_SAMPLES;
		Shadow += DirLightShadowMaps.SampleCmpLevelZero(ShadowMapSampler0, TexCoord.xy + (PoissonDisk[Index] * DiskRadius), CompareDepth);
	}
	
	Shadow = Shadow / POISSON_SAMPLES;
	return min(Shadow, 1.0f);
}
#elif ENABLE_VSM
float CalculateVSM(float2 TexCoords, float CompareDepth)
{
	float2 Moments = (float2)0;
	
	[unroll]
	for (int x = -PCF_RANGE; x <= PCF_RANGE; x++)
	{
		[unroll]
		for (int y = -PCF_RANGE; y <= PCF_RANGE; y++)
		{
			Moments += DirLightShadowMaps.Sample(ShadowMapSampler1, TexCoords, int2(x, y)).rg;
		}
	}

	Moments = Moments / (PCF_WIDTH * PCF_WIDTH);

  //  Moments = DirLightShadowMaps.Sample(ShadowMapSampler1, TexCoords).rg;
	
	float Variance	= max(Moments.y - Moments.x * Moments.x, MIN_VALUE);
	float P			= Moments.x - CompareDepth;
	float Md_2		= P * P;
	float PMax		= Linstep(0.2f, 1.0f, Variance / (Variance + Md_2));
	return min(max(P, PMax), 1.0f);
}
#else
float CalculateStandardShadow(float2 Texcoords, float CompareDepth)
{
	float Shadow = 0.0f;
	
	[unroll]
	for (int x = -PCF_RANGE; x <= PCF_RANGE; x++)
	{
		[unroll]
		for (int y = -PCF_RANGE; y <= PCF_RANGE; y++)
		{
			Shadow += DirLightShadowMaps.SampleCmpLevelZero(ShadowMapSampler0, Texcoords, CompareDepth, int2(x, y)).r;
		}
	}

	Shadow /= (PCF_WIDTH * PCF_WIDTH);
	return min(Shadow, 1.0f);
}
#endif

float CalculateDirLightShadow(float4 LightSpacePosition, float3 WorldPosition, float3 InNormal, float3 InLightDir, float MaxShadowBias, float MinShadowBias)
{
	float3 ProjCoords = LightSpacePosition.xyz / LightSpacePosition.w;
	ProjCoords.xy	= (ProjCoords.xy * 0.5f) + 0.5f;
	ProjCoords.y	= 1.0f - ProjCoords.y;
	
	float Depth = ProjCoords.z;
	if (Depth >= 1.0f)
	{
		return 1.0f;
	}
	
#if ENABLE_POISSON_FILTERING
	float ShadowBias	= max(MaxShadowBias * (1.0f - (dot(InNormal, InLightDir))), MinShadowBias);
	float BiasedDepth	= (Depth - ShadowBias);
	return CalculatePoissonShadow(WorldPosition, ProjCoords.xy, BiasedDepth, 10000.0f);
#elif ENABLE_VSM
	return CalculateVSM(ProjCoords.xy, Depth);
#else
	float ShadowBias	= max(MaxShadowBias * (1.0f - (dot(InNormal, InLightDir))), MinShadowBias);
	float BiasedDepth	= (Depth - ShadowBias);
	return CalculateStandardShadow(ProjCoords.xy, BiasedDepth);
#endif
}

#define OFFSET_SAMPLES	20
#define SAMPLES			2

static const float3 SampleOffsetDirections[OFFSET_SAMPLES] =
{
	float3(1.0f, 1.0f, 1.0f), float3(1.0f, -1.0f, 1.0f), float3(-1.0f, -1.0f, 1.0f), float3(-1.0f, 1.0f, 1.0f),
	float3(1.0f, 1.0f, -1.0f), float3(1.0f, -1.0f, -1.0f), float3(-1.0f, -1.0f, -1.0f), float3(-1.0f, 1.0f, -1.0f),
	float3(1.0f, 1.0f, 0.0f), float3(1.0f, -1.0f, 0.0f), float3(-1.0f, -1.0f, 0.0f), float3(-1.0f, 1.0f, 0.0f),
	float3(1.0f, 0.0f, 1.0f), float3(-1.0f, 0.0f, 1.0f), float3(1.0f, 0.0f, -1.0f), float3(-1.0f, 0.0f, -1.0f),
	float3(0.0f, 1.0f, 1.0f), float3(0.0f, -1.0f, 1.0f), float3(0.0f, -1.0f, -1.0f), float3(0.0f, 1.0f, -1.0f)
};


float CalculatePointLightShadow(float3 WorldPosition, float3 LightPosition, float3 InNormal, float MaxShadowBias, float MinShadowBias, float FarPlane)
{
	float3 DirToLight = WorldPosition - LightPosition;
	float3 LightDir = normalize(LightPosition - WorldPosition);
	float Depth		= length(DirToLight) / FarPlane;

	float ShadowBias	= max(MaxShadowBias * (1.0f - (dot(InNormal, LightDir))), MinShadowBias);
	float BiasedDepth	= (Depth - ShadowBias);
	
	float Shadow = 0.0f;
	const float DiskRadius = (0.4f + (Depth)) / FarPlane;
	
	[unroll]
	for (int i = 0; i < SAMPLES; i++)
	{
		int Index = int(float(OFFSET_SAMPLES) * Random(floor(WorldPosition.xyz * 1000.0f), i)) % OFFSET_SAMPLES;
		Shadow += PointLightShadowMaps.SampleCmpLevelZero(ShadowMapSampler0, DirToLight + SampleOffsetDirections[Index] * DiskRadius, BiasedDepth);
	}
	
	Shadow = Shadow / SAMPLES;
	return min(Shadow, 1.0f);
}

// VertexShader
struct VSInput
{
	float3 Position	: POSITION0;
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord	: TEXCOORD0;
};

struct VSOutput
{
	float3 WorldPosition	: POSITION0;
	float3 Normal			: NORMAL0;
#ifdef NORMAL_MAPPING_ENABLED
	float3 Tangent		: TANGENT0;
	float3 Bitangent	: BITANGENT0;
#endif
	float2 TexCoord : TEXCOORD0;
#ifdef PARALLAX_MAPPING_ENABLED
	float3 TangentViewPos	: TANGENTVIEWPOS0;
	float3 TangentPosition	: TANGENTPOSITION0;
#endif
	float4 Position : SV_Position;
};

VSOutput VSMain(VSInput Input)
{
	VSOutput Output;
	
	float3 Normal = normalize(mul(float4(Input.Normal, 0.0f), TransformBuffer.Transform).xyz);
	Output.Normal = Normal;
	
#ifdef NORMAL_MAPPING_ENABLED
	float3 Tangent	= normalize(mul(float4(Input.Tangent, 0.0f), TransformBuffer.Transform).xyz);
	Tangent			= normalize(Tangent - dot(Tangent, Normal) * Normal);
	Output.Tangent	= Tangent;
	
	float3 Bitangent = normalize(cross(Output.Tangent, Output.Normal));
	Output.Bitangent = Bitangent;
#endif

	Output.TexCoord = Input.TexCoord;

	float4 WorldPosition	= mul(float4(Input.Position, 1.0f), TransformBuffer.Transform);
	Output.Position			= mul(WorldPosition, CameraBuffer.ViewProjection);
	Output.WorldPosition	= WorldPosition.xyz;

#ifdef PARALLAX_MAPPING_ENABLED
	float3x3 TBN	= float3x3(Tangent, Bitangent, Normal);
	TBN				= transpose(TBN);
	
	Output.TangentViewPos	= mul(CameraBuffer.Position, TBN);
	Output.TangentPosition	= mul(WorldPosition.xyz, TBN);
#endif	

	return Output;
}

// PixelShader
struct PSInput
{
	float3 WorldPosition	: POSITION0;
	float3 Normal			: NORMAL0;
#ifdef NORMAL_MAPPING_ENABLED
	float3 Tangent		: TANGENT0;
	float3 Bitangent	: BITANGENT0;
#endif
	float2 TexCoord : TEXCOORD0;
#ifdef PARALLAX_MAPPING_ENABLED
	float3 TangentViewPos	: TANGENTVIEWPOS0;
	float3 TangentPosition	: TANGENTPOSITION0;
#endif
};

#ifdef PARALLAX_MAPPING_ENABLED
static const float HEIGHT_SCALE = 0.03f;

float SampleHeightMap(float2 TexCoords)
{
	return 1.0f - HeightMap.Sample(MaterialSampler, TexCoords).r;
}

float2 ParallaxMapping(float2 TexCoords, float3 ViewDir)
{
	const float MinLayers	= 32;
	const float MaxLayers	= 64;

	float NumLayers		= lerp(MaxLayers, MinLayers, abs(dot(float3(0.0f, 0.0f, 1.0f), ViewDir)));
	float LayerDepth	= 1.0f / NumLayers;
	
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
#endif

float4 PSMain(PSInput Input) : SV_Target0
{
	float2 TexCoords = Input.TexCoord;
	TexCoords.y = 1.0f - TexCoords.y;
	
#ifdef PARALLAX_MAPPING_ENABLED
	if (MaterialBuffer.EnableHeight != 0)
	{
		float3 ViewDir	= normalize(Input.TangentViewPos - Input.TangentPosition);
		TexCoords		= ParallaxMapping(TexCoords, ViewDir);
		//if (TexCoords.x > 1.0f || TexCoords.y > 1.0f || TexCoords.x < 0.0f || TexCoords.y < 0.0f)
		//{
		//	discard;
		//}
	}
#endif

	float3 SampledAlbedo = ApplyGamma(AlbedoMap.Sample(MaterialSampler, TexCoords).rgb) * MaterialBuffer.Albedo;
	
#ifdef NORMAL_MAPPING_ENABLED
	float3 SampledNormal	= NormalMap.Sample(MaterialSampler, TexCoords).rgb;
	SampledNormal			= UnpackNormal(SampledNormal);
	
	float3 Tangent		= normalize(Input.Tangent);
	float3 Bitangent	= normalize(Input.Bitangent);
	float3 Normal		= normalize(Input.Normal);
	const float3 MappedNormal = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);
#else
	const float3 MappedNormal = normalize(Input.Normal);
#endif	

	const float AO			= AOMap.Sample(MaterialSampler, TexCoords) * MaterialBuffer.AO;
	const float Metallic	= MetallicMap.Sample(MaterialSampler, TexCoords) * MaterialBuffer.Metallic;
	const float Roughness	= RoughnessMap.Sample(MaterialSampler, TexCoords) * MaterialBuffer.Roughness;
	const float FinalRoughness	= min(max(Roughness, MIN_ROUGHNESS), MAX_ROUGHNESS);
	const float	Alpha			= AlphaMask.Sample(MaterialSampler, TexCoords);
	
	const float3 WorldPosition	= Input.WorldPosition;
	const float3 Norm			= MappedNormal;
	const float3 ViewDir		= normalize(CameraBuffer.Position - WorldPosition);
	
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, SampledAlbedo, Metallic);

	// Reflectance equation
	float DotNV = max(dot(Norm, ViewDir), 0.0f);
	float3 L0 = float3(0.0f, 0.0f, 0.0f);
	
	// PointLight
	{
		const float3 LightPosition	= PointLightBuffer.Position;
		const float ShadowBias		= PointLightBuffer.ShadowBias;
		const float MaxShadowBias	= PointLightBuffer.MaxShadowBias;
		const float FarPlane		= PointLightBuffer.FarPlane;
		
		float Shadow = CalculatePointLightShadow(WorldPosition, LightPosition, Norm, MaxShadowBias, ShadowBias, FarPlane);
		if (Shadow > EPSILON)
		{
			float3 LightDir		= normalize(LightPosition - WorldPosition);
			float3 HalfVec		= normalize(ViewDir + LightDir);
			float Distance		= length(LightPosition - WorldPosition);
			float Attenuation	= 1.0f / (Distance * Distance);
			float3 Radiance		= PointLightBuffer.Color * Attenuation;

			// Calculate per-light radiance
			L0 += CalcRadiance(F0, Norm, ViewDir, LightDir, Radiance, SampledAlbedo, Roughness, Metallic) * Shadow;
		}
	}
	
	// DirectionalLight
	{
		const float3 LightDir			= normalize(-DirLightBuffer.Direction);
		const float4 LightSpacePosition	= mul(float4(WorldPosition, 1.0f), DirLightBuffer.LightMatrix);
		const float ShadowBias			= DirLightBuffer.ShadowBias;
		const float MaxShadowBias		= DirLightBuffer.MaxShadowBias;
		
		float Shadow = CalculateDirLightShadow(LightSpacePosition, WorldPosition, Norm, LightDir, MaxShadowBias, ShadowBias);
		if (Shadow > EPSILON)
		{
			float3 HalfVec = normalize(ViewDir + LightDir);
			float3 Radiance = DirLightBuffer.Color;

			// Calculate per-light radiance
			L0 += CalcRadiance(F0, Norm, ViewDir, LightDir, Radiance, SampledAlbedo, Roughness, Metallic) * Shadow;
		}
	}
	
	float3 F_IBL	= FresnelSchlickRoughness(DotNV, F0, Roughness);
	float3 Ks_IBL	= F_IBL;
	float3 Kd_IBL	= 1.0f - Ks_IBL;
	Kd_IBL *= 1.0 - Metallic;
	
	float3 Irradiance	= IrradianceMap.Sample(IrradianceSampler, Norm).rgb;
	float3 IBL_Diffuse	= Irradiance * SampledAlbedo * Kd_IBL;
	
	const float MAX_MIPLEVEL = 6.0f;
	float3 Reflection = reflect(-ViewDir, Norm);
#ifdef RAYTRACING_ENABLED
	float3 Prefiltered	= 
		(SpecularIrradianceMap.SampleLevel(IrradianceSampler, Reflection, Roughness * MAX_MIPLEVEL).rgb * Roughness) + 
		(SampledReflection * (1.0f - Roughness));
#else
	float3 Prefiltered = SpecularIrradianceMap.SampleLevel(IrradianceSampler, Reflection, Roughness * MAX_MIPLEVEL).rgb;
#endif
	float2 IntegrationBRDF	= IntegrationLUT.Sample(LUTSampler, float2(DotNV, Roughness)).rg;
	float3 IBL_Specular		= Prefiltered * (F_IBL * IntegrationBRDF.x + IntegrationBRDF.y);
	
	float3 Ambient	= (IBL_Diffuse + IBL_Specular) * AO;
	float3 Color	= Ambient + L0;
	
	float3 FinalColor = ApplyGammaCorrectionAndTonemapping(Color);
	return float4(FinalColor, Alpha);
}