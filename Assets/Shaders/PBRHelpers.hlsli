#ifndef PBR_HELPERS_HLSLI
#define PBR_HELPERS_HLSLI

#include "Constants.hlsli"
#include "Helpers.hlsli"
#include "Halton.hlsli"

// ImportanceSample GGX
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
    float Alpha    = Roughness * Roughness;
    float Phi      = 2 * PI * Xi.x;
    float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (Alpha * Alpha - 1.0) * Xi.y));
    float SinTheta = sqrt(1.0 - CosTheta * CosTheta);
    
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    float3 Up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
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
    float NDotH  = max(dot(N, H), 0.0);
    float Denominator = NDotH * NDotH * (Alpha2 - 1.0) + 1.0;
    return Alpha2 / max(PI * Denominator * Denominator, 0.0000001);
}

//float3 FresnelSchlick(float CosTheta, float3 F0)
//{
//    return F0 + (1.0f - F0) * pow(1.0f - CosTheta, 5.0f);
//}

//float3 FresnelSchlickRoughness(float CosTheta, float3 F0, float Roughness)
//{
//    float R = 1.0f - Roughness;
//    return F0 + (max(float3(R, R, R), F0) - F0) * pow(1.0f - CosTheta, 5.0f);
//}

// Fresnel Schlick
float3 FresnelSchlick(float3 F0, float3 V, float3 H)
{
    float VDotH = max(dot(V, H), 0.0);
    float Exp   = (-5.55473 * VDotH - 6.98316) * VDotH;
    return F0 + (1.0 - F0) * exp2(Exp);
}

float3 FresnelSchlick_Roughness(float3 F0, float3 V, float3 H, float Roughness)
{
    float R     = 1.0 - Roughness;
    float VDotH = max(dot(V, H), 0.0);
    float Exp   = (-5.55473 * VDotH - 6.98316) * VDotH;
    return F0 + (max(R, F0) - F0) * exp2(Exp);
}

// Geometry Smitch
float GeometrySmithGGX1(float3 N, float3 V, float Roughness)
{
    float Roughness1 = Roughness + 1;
    float K     = (Roughness1 * Roughness1) / 8.0;
    float NDotV = max(dot(N, V), 0.0);
    return NDotV / max(NDotV * (1.0 - K) + K, 0.0000001);
}

float GeometrySmithGGX(float3 N, float3 L, float3 V, float Roughness)
{
    return GeometrySmithGGX1(N, L, Roughness) * GeometrySmithGGX1(N, V, Roughness);
}

float GeometrySmithGGX1_IBL(float3 N, float3 V, float Roughness)
{
    float K     = (Roughness * Roughness) / 2.0;
    float NDotV = max(dot(N, V), 0.0);
    return NDotV / max(NDotV * (1.0 - K) + K, 0.0000001);
}

float GeometrySmithGGX_IBL(float3 N, float3 L, float3 V, float Roughness)
{
    return GeometrySmithGGX1_IBL(N, L, Roughness) * GeometrySmithGGX1_IBL(N, V, Roughness);
}

// Radiance
float3 DirectRadiance(float3 F0, float3 N, float3 V, float3 L, float3 Radiance, float3 Albedo, float Roughness, float Metallic)
{
    // Lambert Diffuse BRDF
    float3 DiffBRDF = Albedo / PI;
    
    // Cook-Torrance Specular BRDF
    const float NDotL = max(dot(N, L), 0.0);
    const float NDotV = max(dot(N, V), 0.0);
    const float M     = 1.0 - Metallic;

    const float3 H     = normalize(V + L);
    float3 F           = FresnelSchlick(F0, V, H);
    float3 Numerator   = DistributionGGX(N, H, Roughness) * F * GeometrySmithGGX(N, L, V, Roughness);
    float3 Denominator = 4.0 * NDotL * NDotV;
    float3 SpecBRDF    = Numerator / max(Denominator, 0.0000001);
    
    float3 Ks = F;
    float3 Kd = (1.0 - Ks) * M;
    
    return (Kd * DiffBRDF + Ks * SpecBRDF) * Radiance * NDotL;
}

#endif