#ifndef SHADOW_HELPERS_HLSLI
#define SHADOW_HELPERS_HLSLI

#include "Structs.hlsli"
#include "Helpers.hlsli"

/*
* Calculate PointLight Shadow
*/

#define POINT_LIGHT_SAMPLES 4
#define OFFSET_SAMPLES	    20

static const float3 SampleOffsetDirections[OFFSET_SAMPLES] =
{
    float3(1.0f, 1.0f,  1.0f), float3( 1.0f, -1.0f,  1.0f), float3(-1.0f, -1.0f,  1.0f), float3(-1.0f, 1.0f,  1.0f),
	float3(1.0f, 1.0f, -1.0f), float3( 1.0f, -1.0f, -1.0f), float3(-1.0f, -1.0f, -1.0f), float3(-1.0f, 1.0f, -1.0f),
	float3(1.0f, 1.0f,  0.0f), float3( 1.0f, -1.0f,  0.0f), float3(-1.0f, -1.0f,  0.0f), float3(-1.0f, 1.0f,  0.0f),
	float3(1.0f, 0.0f,  1.0f), float3(-1.0f,  0.0f,  1.0f), float3( 1.0f,  0.0f, -1.0f), float3(-1.0f, 0.0f, -1.0f),
	float3(0.0f, 1.0f,  1.0f), float3( 0.0f, -1.0f,  1.0f), float3( 0.0f, -1.0f, -1.0f), float3( 0.0f, 1.0f, -1.0f)
};

float PointLightShadowFactor(
    in TextureCube<float> ShadowMap,
    in SamplerComparisonState Sampler,
    float3 WorldPosition, 
    float3 Normal,
    PointLight Light)
{
    const float3 DirToLight   = WorldPosition - Light.Position;
    const float3 LightDir     = normalize(Light.Position - WorldPosition);

    const float ShadowBias = max(Light.MaxShadowBias * (1.0f - (max(dot(Normal, LightDir), 0.0f))), Light.ShadowBias);
    float Depth = length(DirToLight) / Light.FarPlane;
    Depth = (Depth - ShadowBias);
	
    float Shadow = 0.0f;
    const float DiskRadius = (0.4f + (Depth)) / Light.FarPlane;
	
	[unroll]
    for (int i = 0; i < POINT_LIGHT_SAMPLES; i++)
    {
        const int Index = int(float(OFFSET_SAMPLES) * Random(floor(WorldPosition.xyz * 1000.0f), i)) % OFFSET_SAMPLES;
        Shadow += ShadowMap.SampleCmpLevelZero(Sampler, DirToLight + SampleOffsetDirections[Index] * DiskRadius, Depth);
    }
	
    Shadow = Shadow / POINT_LIGHT_SAMPLES;
    return min(Shadow, 1.0f);
}

float PointLightShadowFactor(
    in TextureCubeArray<float> ShadowMap, float Index,
    in SamplerComparisonState Sampler,
    float3 WorldPosition,
    float3 Normal,
    PointLight Light)
{
    const float3 DirToLight = WorldPosition - Light.Position;
    const float3 LightDir   = normalize(Light.Position - WorldPosition);

    const float ShadowBias = max(Light.MaxShadowBias * (1.0f - (max(dot(Normal, LightDir), 0.0f))), Light.ShadowBias);
    float Depth = length(DirToLight) / Light.FarPlane;
    Depth       = (Depth - ShadowBias);
	
    float Shadow = 0.0f;
    const float DiskRadius = (0.4f + (Depth)) / Light.FarPlane;
	
	[unroll]
    for (int i = 0; i < POINT_LIGHT_SAMPLES; i++)
    {
        const int OffsetIndex = int(float(OFFSET_SAMPLES) * Random(floor(WorldPosition.xyz * 1000.0f), i)) % OFFSET_SAMPLES;
        
        const float3 SampleVec = DirToLight + SampleOffsetDirections[OffsetIndex] * DiskRadius;
        Shadow += ShadowMap.SampleCmpLevelZero(Sampler, float4(SampleVec, Index), Depth);
    }
	
    Shadow = Shadow / POINT_LIGHT_SAMPLES;
    return min(Shadow, 1.0f);
}

/*
* Calculate DirectionalLight Shadows
*/

#define PCF_RANGE	2
#define PCF_WIDTH	float((PCF_RANGE * 2) + 1)

float StandardShadow(
    in Texture2D<float> ShadowMap,
    in SamplerComparisonState Sampler,
    float2 Texcoords,
    float CompareDepth)
{
    float Shadow = 0.0f;
	
	[unroll]
    for (int x = -PCF_RANGE; x <= PCF_RANGE; x++)
    {
		[unroll]
        for (int y = -PCF_RANGE; y <= PCF_RANGE; y++)
        {
            Shadow += ShadowMap.SampleCmpLevelZero(Sampler, Texcoords, CompareDepth, int2(x, y)).r;
        }
    }

    Shadow = Shadow / (PCF_WIDTH * PCF_WIDTH);
    return min(Shadow, 1.0f);
}

float DirectionalLightShadowFactor(
    in Texture2D<float> ShadowMap,
    in SamplerComparisonState Sampler,
    float3 WorldPosition, 
    float3 N,
    DirectionalLight Light)
{
    float4 LightSpacePosition = mul(float4(WorldPosition, 1.0f), Light.LightMatrix);
    float3 L = normalize(-Light.Direction);
    
    float3 ProjCoords   = LightSpacePosition.xyz / LightSpacePosition.w;
    ProjCoords.xy       = (ProjCoords.xy * 0.5f) + 0.5f;
    ProjCoords.y        = 1.0f - ProjCoords.y;
	
    float Depth = ProjCoords.z;
    if (Depth >= 1.0f)
    {
        return 1.0f;
    }
	
    float ShadowBias    = max(Light.MaxShadowBias * (1.0f - (dot(N, L))), Light.ShadowBias);
    float BiasedDepth   = (Depth - ShadowBias);
    return StandardShadow(ShadowMap, Sampler, ProjCoords.xy, BiasedDepth);
}

#endif