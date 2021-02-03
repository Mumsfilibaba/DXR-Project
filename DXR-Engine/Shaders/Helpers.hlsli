#ifndef HELPERS_HLSLI
#define HELPERS_HLSLI

#include "Constants.hlsli"

float2 Float2(float Single)
{
    return float2(Single, Single);
}

float3 Float3(float Single)
{
    return float3(Single, Single, Single);
}

float4 Float4(float Single)
{
    return float4(Single, Single, Single, Single);
}

float Luma(float3 Color)
{
    return sqrt(dot(Color, float3(0.299f, 0.587f, 0.114f)));
}

float4 CreatePlane(float3 Q, float3 R)
{
    float3 N = normalize(cross(Q, R));
    return float4(N, 0);
}

float GetSignedDistanceFromPlane(float3 P, float4 Plane)
{
    return dot(Plane.xyz, P);
}

uint DivideByMultiple(uint Value, uint Alignment)
{
    return ((Value + Alignment - 1) / Alignment);
}

float Random(float3 Seed, int i)
{
    float4  Seed4   = float4(Seed, i);
    float   Dot     = dot(Seed4, float4(12.9898f, 78.233f, 45.164f, 94.673f));
    return frac(sin(Dot) * 43758.5453f);
}

float Linstep(float Low, float High, float P)
{
    return saturate((P - Low) / (High - Low));
}

float Lerp(float A, float B, float P)
{
    return (-P * B) + ((A * P) + B);
}

float3 Lerp(float3 A, float3 B, float P)
{
    return (Float3(-P) * B) + ((A * Float3(P)) + B);
}

float Depth_ProjToView(float Depth, float4x4 ProjectionInverse)
{
    return 1.0f / (Depth * ProjectionInverse._34 + ProjectionInverse._44);
}

float3 Float3_ProjToView(float3 P, float4x4 ProjectionInverse)
{
    float4 ViewP = mul(float4(P, 1.0f), ProjectionInverse);
    return (ViewP / ViewP.w).xyz;
}

float3 PositionFromDepth(float Depth, float2 TexCoord, float4x4 ProjectionInverse)
{
    float z = Depth;
    float x = TexCoord.x * 2.0f - 1.0f;
    float y = (1.0f - TexCoord.y) * 2.0f - 1.0f;

    float4 ProjectedPos     = float4(x, y, z, 1.0f);
    float4 FinalPosition    = mul(ProjectedPos, ProjectionInverse);
    
    return FinalPosition.xyz / FinalPosition.w;
}

float3 ApplyGamma(float3 Color)
{
    return pow(Color, Float3(GAMMA));
}

float3 ApplyGammaInv(float3 InputColor)
{
    return pow(InputColor, Float3(1.0f / GAMMA));
}

float3 ReinhardMapping(float3 Color, float Intensity)
{
    return Color / (Color + Float3(Intensity));
}

float3 ApplyGammaCorrectionAndTonemapping(float3 Color)
{
    const float INTENSITY = 0.75f;
    return ApplyGammaInv(ReinhardMapping(Color, INTENSITY));
}

float3 ApplyNormalMapping(float3 MappedNormal, float3 Normal, float3 Tangent, float3 Bitangent)
{
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    return normalize(mul(MappedNormal, TBN));
}

float3 UnpackNormal(float3 SampledNormal)
{
    return normalize((SampledNormal * 2.0f) - 1.0f);
}

float3 PackNormal(float3 Normal)
{
    return (normalize(Normal) + 1.0f) * 0.5f;
}

#endif