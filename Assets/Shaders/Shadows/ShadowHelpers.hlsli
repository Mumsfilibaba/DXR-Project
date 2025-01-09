#ifndef SHADOW_HELPERS_HLSLI
#define SHADOW_HELPERS_HLSLI
#include "../Structs.hlsli"
#include "../Helpers.hlsli"
#include "../Random.hlsli"
#include "../Poisson.hlsli"

/*
* Calculate PointLight Shadow
*/

#define POINT_LIGHT_SAMPLES (4)
#define NUM_OFFSET_SAMPLES (20)

static const float3 SampleOffsetDirections[NUM_OFFSET_SAMPLES] =
{
    float3(1.0f, 1.0f,  1.0f), float3( 1.0f, -1.0f,  1.0f), float3(-1.0f, -1.0f,  1.0f), float3(-1.0f, 1.0f,  1.0f),
    float3(1.0f, 1.0f, -1.0f), float3( 1.0f, -1.0f, -1.0f), float3(-1.0f, -1.0f, -1.0f), float3(-1.0f, 1.0f, -1.0f),
    float3(1.0f, 1.0f,  0.0f), float3( 1.0f, -1.0f,  0.0f), float3(-1.0f, -1.0f,  0.0f), float3(-1.0f, 1.0f,  0.0f),
    float3(1.0f, 0.0f,  1.0f), float3(-1.0f,  0.0f,  1.0f), float3( 1.0f,  0.0f, -1.0f), float3(-1.0f, 0.0f, -1.0f),
    float3(0.0f, 1.0f,  1.0f), float3( 0.0f, -1.0f,  1.0f), float3( 0.0f, -1.0f, -1.0f), float3( 0.0f, 1.0f, -1.0f)
};

float PointLightShadowFactor(in TextureCube<float> ShadowMap, in SamplerComparisonState Sampler, float3 WorldPosition, float3 Normal, FShadowPointLight Light, FPositionRadius LightPos)
{
    const float3 DirToLight = WorldPosition - LightPos.Position;
    const float3 LightDir   = normalize(LightPos.Position - WorldPosition);

    const float ShadowBias = max(Light.MaxShadowBias * (1.0f - (max(dot(Normal, LightDir), 0.0f))), Light.ShadowBias);
    float Depth = length(DirToLight) / Light.FarPlane;
    Depth = (Depth - ShadowBias);
    
    float Shadow = 0.0f;
    const float DiskRadius = (0.4f + (Depth)) / Light.FarPlane;
    
    [unroll]
    for (int i = 0; i < POINT_LIGHT_SAMPLES; i++)
    {
        const int Index = int(float(NUM_OFFSET_SAMPLES) * Random(floor(WorldPosition.xyz * 1000.0f), i)) % NUM_OFFSET_SAMPLES;
        Shadow += ShadowMap.SampleCmpLevelZero(Sampler, DirToLight + SampleOffsetDirections[Index] * DiskRadius, Depth);
    }
    
    Shadow = Shadow / POINT_LIGHT_SAMPLES;
    return min(Shadow, 1.0f);
}

// TODO: Reuse code form other func?
float PointLightShadowFactor(in TextureCubeArray<float> ShadowMap, float Index, in SamplerComparisonState Sampler, float3 WorldPosition, float3 Normal, FShadowPointLight Light, FPositionRadius LightPos)
{
    const float3 DirToLight = WorldPosition - LightPos.Position;
    const float3 LightDir   = normalize(LightPos.Position - WorldPosition);

    const float ShadowBias = max(Light.MaxShadowBias * (1.0f - (max(dot(Normal, LightDir), 0.0f))), Light.ShadowBias);
    float Depth = length(DirToLight) / Light.FarPlane;
    Depth       = (Depth - ShadowBias);
    
    float Shadow = 0.0f;
    const float DiskRadius = (0.4f + (Depth)) / Light.FarPlane;
    
    [unroll]
    for (int i = 0; i < POINT_LIGHT_SAMPLES; i++)
    {
        const int OffsetIndex = int(float(NUM_OFFSET_SAMPLES) * Random(floor(WorldPosition.xyz * 1000.0f), i)) % NUM_OFFSET_SAMPLES;
        
        const float3 SampleVec = DirToLight + SampleOffsetDirections[OffsetIndex] * DiskRadius;
        Shadow += ShadowMap.SampleCmpLevelZero(Sampler, float4(SampleVec, Index), Depth);
    }
    
    Shadow = Shadow / POINT_LIGHT_SAMPLES;
    return min(Shadow, 1.0f);
}

#endif