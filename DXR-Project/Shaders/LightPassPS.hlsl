#include "PBRCommon.hlsli"

#if ENABLE_RAYTRACING
#define RAYTRACING_ENABLED
#endif

// Resources
struct PSInput
{
	float2 TexCoord : TEXCOORD0;
};

Texture2D<float4>	Albedo					: register(t0, space0);
Texture2D<float4>	Normal					: register(t1, space0);
Texture2D<float4>	Material				: register(t2, space0);
Texture2D<float>	DepthStencil			: register(t3, space0);
Texture2D<float4>	DXRReflection			: register(t4, space0);
TextureCube<float4>	IrradianceMap			: register(t5, space0);
TextureCube<float4> SpecularIrradianceMap	: register(t6, space0);
Texture2D<float4>	IntegrationLUT			: register(t7, space0);
Texture2D<float2>	DirLightShadowMaps		: register(t8, space0);
TextureCube<float>	PointLightShadowMaps	: register(t9, space0);
Texture2D<float>	SSAO					: register(t10, space0);

SamplerState GBufferSampler		: register(s0, space0);
SamplerState LUTSampler			: register(s1, space0);
SamplerState IrradianceSampler	: register(s2, space0);

SamplerComparisonState	ShadowMapSampler0	: register(s3, space0);
SamplerState			ShadowMapSampler1	: register(s4, space0);

ConstantBuffer<Camera>				CameraBuffer		: register(b0, space0);
ConstantBuffer<PointLight>			PointLightBuffer	: register(b1, space0);
ConstantBuffer<DirectionalLight>	DirLightBuffer		: register(b2, space0);

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
	float3(1.0f, 1.0f,  1.0f),	float3( 1.0f, -1.0f,  1.0f),	float3(-1.0f, -1.0f,  1.0f),	float3(-1.0f, 1.0f,  1.0f),
	float3(1.0f, 1.0f, -1.0f),	float3( 1.0f, -1.0f, -1.0f),	float3(-1.0f, -1.0f, -1.0f),	float3(-1.0f, 1.0f, -1.0f),
	float3(1.0f, 1.0f,  0.0f),	float3( 1.0f, -1.0f,  0.0f),	float3(-1.0f, -1.0f,  0.0f),	float3(-1.0f, 1.0f,  0.0f),
	float3(1.0f, 0.0f,  1.0f),	float3(-1.0f,  0.0f,  1.0f),	float3( 1.0f,  0.0f, -1.0f),	float3(-1.0f, 0.0f, -1.0f),
	float3(0.0f, 1.0f,  1.0f),	float3( 0.0f, -1.0f, 1.0f),		float3( 0.0f, -1.0f, -1.0f),	float3( 0.0f, 1.0f, -1.0f)
};

float CalculatePointLightShadow(float3 WorldPosition, float3 LightPosition, float3 InNormal, float MaxShadowBias, float MinShadowBias, float FarPlane)
{
	float3 DirToLight	= WorldPosition - LightPosition;
	float3 LightDir		= normalize(LightPosition - WorldPosition);
	float Depth			= length(DirToLight) / FarPlane;

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

// Main
float4 Main(PSInput Input) : SV_TARGET
{
	const float2 TexCoord = Input.TexCoord;
	
	float Depth = DepthStencil.Sample(GBufferSampler, TexCoord).r;
	if (Depth >= 1.0f)
	{
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	
    float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInverse);
	float3 SampledAlbedo = Albedo.Sample(GBufferSampler, TexCoord).rgb;
#ifdef RAYTRACING_ENABLED
	float3 SampledReflection	= DXRReflection.Sample(LUTSampler, TexCoord).rgb;
#endif
	float3 SampledMaterial	= Material.Sample(GBufferSampler, TexCoord).rgb;
	float3 SampledNormal	= Normal.Sample(GBufferSampler, TexCoord).rgb;
	float ScreenSpaceAO		= SSAO.Sample(GBufferSampler, TexCoord);
	
	const float3 Norm		= UnpackNormal(SampledNormal);
	const float3 ViewDir	= normalize(CameraBuffer.Position - WorldPosition);
	const float Roughness	= SampledMaterial.r;
	const float Metallic	= SampledMaterial.g;
	const float SampledAO	= SampledMaterial.b * ScreenSpaceAO;
	
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, SampledAlbedo, Metallic);

	// Reflectance equation
	float	DotNV	= max(dot(Norm, ViewDir), 0.0f);
	float3	L0		= float3(0.0f, 0.0f, 0.0f);
	
	// PointLight
	{
		const float3 LightPosition	= PointLightBuffer.Position;	
		const float ShadowBias		= PointLightBuffer.ShadowBias;
		const float MaxShadowBias	= PointLightBuffer.MaxShadowBias;
		const float FarPlane		= PointLightBuffer.FarPlane;
		
		float Shadow = CalculatePointLightShadow(WorldPosition, LightPosition, Norm, MaxShadowBias, ShadowBias, FarPlane);
		if (Shadow > EPSILON)
		{
			float3	LightDir	= normalize(LightPosition - WorldPosition);
			float3	HalfVec		= normalize(ViewDir + LightDir);
			float	Distance	= length(LightPosition - WorldPosition);
			float	Attenuation	= 1.0f / (Distance * Distance);
			float3	Radiance	= PointLightBuffer.Color * Attenuation;

			// Calculate per-light radiance
			L0 += CalcRadiance(F0, Norm, ViewDir, LightDir, Radiance, SampledAlbedo, Roughness, Metallic) * Shadow;
		}
	}
	
	// DirectionalLight
	{
		const float3 LightDir			= normalize(-DirLightBuffer.Direction);
		const float4 LightSpacePosition	= mul(float4(WorldPosition, 1.0f), DirLightBuffer.LightMatrix);
		const float ShadowBias		= DirLightBuffer.ShadowBias;
		const float MaxShadowBias	= DirLightBuffer.MaxShadowBias;
		
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
	float3 Reflection	= reflect(-ViewDir, Norm);
#ifdef RAYTRACING_ENABLED
	float3 Prefiltered	= 
		(SpecularIrradianceMap.SampleLevel(IrradianceSampler, Reflection, Roughness * MAX_MIPLEVEL).rgb * Roughness) + 
		(SampledReflection * (1.0f - Roughness));
#else
	float3 Prefiltered = SpecularIrradianceMap.SampleLevel(IrradianceSampler, Reflection, Roughness * MAX_MIPLEVEL).rgb;
#endif
	float2 IntegrationBRDF	= IntegrationLUT.Sample(LUTSampler, float2(DotNV, Roughness)).rg;
	float3 IBL_Specular		= Prefiltered * (F_IBL * IntegrationBRDF.x + IntegrationBRDF.y);
	
	float3 Ambient	= (IBL_Diffuse + IBL_Specular) * SampledAO;
	float3 Color	= Ambient + L0;
	
	float3	FinalColor	= ApplyGammaCorrectionAndTonemapping(Color);
	float	Luminance	= CalculateLuminance(FinalColor);
    //return float4(ToFloat3(ScreenSpaceAO), Luminance);
    return float4(FinalColor, Luminance);
}