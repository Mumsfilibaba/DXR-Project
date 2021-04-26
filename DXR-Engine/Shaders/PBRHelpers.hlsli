#ifndef PBR_HELPERS_HLSLI
#define PBR_HELPERS_HLSLI

#include "Constants.hlsli"
#include "Helpers.hlsli"

// ImportanceSample GGX
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
    float Alpha = Roughness * Roughness;
    float Phi   = 2 * PI * Xi.x;
    float CosTheta = sqrt((1.0f - Xi.y) / (1.0f + (Alpha * Alpha - 1.0f) * Xi.y));
    float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
    
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    float3 Up       = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 TangentX = normalize(cross(Up, N));
    float3 TangentY = cross(N, TangentX);
    return normalize(TangentX * H.x + TangentY * H.y + N * H.z);
}

float3 ImportanceSampleGGX_PDF(float2 Xi, float Roughness, float3 N, inout float PDF)
{
    float Alpha  = Roughness * Roughness;
    float Alpha2 = Alpha * Alpha;
    float Phi    = 2 * PI * Xi.x;
    float CosTheta = sqrt((1.0f - Xi.y) / (1.0f + (Alpha2 - 1.0f) * Xi.y));
    float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
    
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    float Denom = CosTheta * CosTheta * (Alpha2 - 1.0f) + 1.0f;
    float D = Alpha2 / (PI * Denom * Denom);
    PDF = D * CosTheta;
    
    float3 Up       = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 TangentX = normalize(cross(Up, N));
    float3 TangentY = cross(N, TangentX);
    return normalize(TangentX * H.x + TangentY * H.y + N * H.z);
}

// GGX Normal Distribution
float DistributionGGX(float3 N, float3 H, float Roughness)
{
    float Alpha  = Roughness * Roughness;
    float Alpha2 = Alpha * Alpha;
    float NdotH  = saturate(dot(N, H));
    float Denom  = NdotH * NdotH * (Alpha2 - 1.0f) + 1.0f;
    return Alpha2 / max(PI * Denom * Denom, 1e-6);
}

// Fresnel Schlick
float3 FresnelSchlick(float3 F0, float3 V, float3 H)
{
    float VdotH = saturate(dot(V, H));
    return F0 + (1.0f - F0) * max(pow(1.0f - VdotH, 5.0f), 1e-6);
}

float3 FresnelSchlick_Roughness(float3 F0, float3 V, float3 H, float Roughness)
{
    float VdotH = saturate(dot(V, H));
    return F0 + (max(Float3(1.0f - Roughness), F0) - F0) * max(pow(1.0f - VdotH, 5.0f), 1e-6);
}

// Geometry Smith
float GeometrySmithGGX(float3 N, float3 L, float3 V, float Roughness)
{
    float r = Roughness + 1;
    float k = (r * r) / 8.0f;
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float GL = NdotL / max(NdotL * (1.0f - k) + k, 1e-6);
    float GV = NdotV / max(NdotV * (1.0f - k) + k, 1e-6);
    return GL * GV;
}

float GeometrySmithGGX_IBL(float3 N, float3 L, float3 V, float Roughness)
{
    float k = (Roughness * Roughness) / 2.0f;
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float GL = NdotL / max(NdotL * (1.0f - k) + k, 1e-6);
    float GV = NdotV / max(NdotV * (1.0f - k) + k, 1e-6);
    return GL * GV;
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
    
    const float NdotL = saturate(dot(N, L));
    const float NdotV = saturate(dot(N, V));
    
    // Cook-Torrance Specular BRDF
    float3 H = normalize(V + L);
    float3 F = FresnelSchlick(F0, V, H);
    float3 Num = DistributionGGX(N, H, Roughness) * F * GeometrySmithGGX(N, L, V, Roughness);
    float3 Den = 4.0f * NdotL * NdotV;
    float3 SpecBRDF = Num / max(Den, 1e-6);
    
    float3 Ks = F;
    float3 Kd = (Float3(1.0f) - Ks) * (1.0f - Metallic);
    
    return (Kd * DiffBRDF + SpecBRDF) * Radiance * NdotL;
}

#endif