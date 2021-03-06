#ifndef PBR_HELPERS_HLSLI
#define PBR_HELPERS_HLSLI

#include "Constants.hlsli"
#include "Helpers.hlsli"

// ImportanceSample GGX
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
    float Alpha    = Roughness * Roughness;
    float Phi      = 2 * PI * Xi.x;
    float CosTheta = sqrt((1.0f - Xi.y) / (1.0f + (Alpha * Alpha - 1.0f) * Xi.y));
    float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
    
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    float3 Up       = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 TangentX = normalize(cross(Up, N));
    float3 TangentY = cross(N, TangentX);
    float3 Sample   = TangentX * H.x + TangentY * H.y + N * H.z;
    return normalize(Sample);
}

// GGX Distribution
float DistributionGGX(float3 N, float3 H, float Roughness)
{
    float Alpha  = Roughness * Roughness;
    float Alpha2 = Alpha * Alpha;
    float NdotH  = saturate(dot(N, H));
    float Denom  = NdotH * NdotH * (Alpha2 - 1.0f) + 1.0f;
    return Alpha2 / (PI * Denom * Denom);
}

// Fresnel Schlick
float3 FresnelSchlick(float3 F0, float3 V, float3 H)
{
    float VdotH = saturate(dot(V, H));
    return F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);
}

float3 FresnelSchlick_Roughness(float3 F0, float3 V, float3 H, float Roughness)
{
    float VdotH = saturate(dot(V, H));
    return F0 + (max(Float3(1.0f - Roughness), F0) - F0) * pow(1.0f - VdotH, 5.0f);
}

// Geometry Smitch
float GeometrySmithGGX1(float3 N, float3 V, float Roughness)
{
    float Roughness1 = Roughness + 1;
    float K     = (Roughness1 * Roughness1) / 8.0f;
    float NdotV = saturate(dot(N, V));
    return NdotV / max(NdotV * (1.0f - K) + K, 0.0000001f);
}

float GeometrySmithGGX(float3 N, float3 L, float3 V, float Roughness)
{
    return GeometrySmithGGX1(N, L, Roughness) * GeometrySmithGGX1(N, V, Roughness);
}

float GeometrySmithGGX1_IBL(float3 N, float3 V, float Roughness)
{
    float K     = (Roughness * Roughness) / 2.0f;
    float NdotV = saturate(dot(N, V));
    return NdotV / max(NdotV * (1.0f - K) + K, 0.0000001f);
}

float GeometrySmithGGX_IBL(float3 N, float3 L, float3 V, float Roughness)
{
    return GeometrySmithGGX1_IBL(N, L, Roughness) * GeometrySmithGGX1_IBL(N, V, Roughness);
}

// Radiance
float3 DirectRadiance(
    float3 F0,
    float3 N,
    float3 V,
    float3 L,
    float3 Radiance,
    float3 Albedo,
    float Roughness,
    float Metallic)
{
    // Lambert Diffuse BRDF
    float3 DiffBRDF = Albedo / PI;
    
    // Cook-Torrance Specular BRDF
    const float3 H     = normalize(V + L);
    float3 F           = FresnelSchlick(F0, V, H);
    float3 Numerator   = DistributionGGX(N, H, Roughness) * F * GeometrySmithGGX(N, L, V, Roughness);
    float3 Denominator = 4.0f * saturate(dot(N, L)) * saturate(dot(N, V));
    float3 SpecBRDF    = Numerator / max(Denominator, 0.0000001f);
    
    float3 Ks = F;
    float3 Kd = (Float3(1.0f) - Ks) * (1.0f - Metallic);
    
    const float NDotL = saturate(dot(N, L));
    return (Kd * DiffBRDF + Ks * SpecBRDF) * Radiance * NDotL;
}

#endif