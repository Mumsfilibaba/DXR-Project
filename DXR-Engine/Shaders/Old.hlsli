// Light Calculations
float3 CalcRadiance(float3 F0, float3 InNormal, float3 InViewDir, float3 InLightDir, float3 InRadiance, float3 InAlbedo, float InRoughness, float InMetallic)
{
    float3 HalfVector = normalize(InViewDir + InLightDir);
	
	// Cook-Torrance BRDF
    float NDF = DistributionGGX(InNormal, HalfVector, InRoughness);
    float G = GeometrySmith(InNormal, InViewDir, InLightDir, InRoughness);
    float3 F = FresnelSchlick(saturate(dot(HalfVector, InViewDir)), F0);
	
    float DotNV = max(dot(InNormal, InViewDir), MIN_VALUE);
    float3 Nominator = NDF * G * F;
    float Denominator = 4.0f * DotNV * max(dot(InNormal, InLightDir), 0.0f);
    float3 Specular = Nominator / max(Denominator, MIN_VALUE);
		
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
    ProjCoords.xy = (ProjCoords.xy * 0.5f) + 0.5f;
    ProjCoords.y = 1.0f - ProjCoords.y;
	
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
    float ShadowBias = max(MaxShadowBias * (1.0f - (dot(InNormal, InLightDir))), MinShadowBias);
    float BiasedDepth = (Depth - ShadowBias);
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
    float Depth = length(DirToLight) / FarPlane;

    float ShadowBias = max(MaxShadowBias * (1.0f - (dot(InNormal, LightDir))), MinShadowBias);
    float BiasedDepth = (Depth - ShadowBias);
	
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