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

bool IsNan(float2 Num)
{
    bool2 Result = isnan(Num);
    return Result.x || Result.y;
}

bool IsNan(float3 Num)
{
    bool3 Result = isnan(Num);
    return Result.x || Result.y || Result.z;
}

bool IsNan(float4 Num)
{
    bool4 Result = isnan(Num);
    return Result.x || Result.y || Result.z || Result.w;
}

bool IsInf(float2 Num)
{
    bool2 Result = isinf(Num);
    return Result.x || Result.y;
}

bool IsInf(float3 Num)
{
    bool3 Result = isinf(Num);
    return Result.x || Result.y || Result.z;
}

bool IsInf(float4 Num)
{
    bool4 Result = isinf(Num);
    return Result.x || Result.y || Result.z || Result.w;
}

bool IsEqual(float2 LHS, float2 RHS)
{
    bool2 Result = LHS == RHS;
    return Result.x || Result.y;
}

bool IsEqual(float3 LHS, float3 RHS)
{
    bool3 Result = LHS == RHS;
    return Result.x || Result.y || Result.z;
}

bool IsEqual(float4 LHS, float4 RHS)
{
    bool4 Result = LHS == RHS;
    return Result.x || Result.y || Result.z || Result.w;
}

float Luma(float3 Color)
{
    return sqrt(dot(Color, float3(0.299f, 0.587f, 0.114f)));
}

float Luminance(float3 Color)
{
    return dot(Color, float3(0.2126f, 0.7152f, 0.0722f));
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

float Linstep(float Low, float High, float P)
{
    return saturate((P - Low) / (High - Low));
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

    float4 ProjectedPos  = float4(x, y, z, 1.0f);
    float4 FinalPosition = mul(ProjectedPos, ProjectionInverse);
    
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

float3 SimpleReinhardMapping(float3 Color, float Intensity)
{
    return Color / (Float3(Intensity) + Color);
}

float3 RTTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 AcesFitted(float3 Color)
{
    const float3x3 InputMatrix =
    {
        { 0.59719f, 0.35458f, 0.04823f },
        { 0.07600f, 0.90834f, 0.01566f },
        { 0.02840f, 0.13383f, 0.83777f },
    };

    const float3x3 OutputMatrix =
    {
        { 1.60475f, -0.53108f, -0.07367f },
        { -0.10208f, 1.10813f, -0.00605f },
        { -0.00327f, -0.07276f, 1.07602f },
    };

    Color = mul(InputMatrix, Color);
    Color = RTTAndODTFit(Color);
    return saturate(mul(OutputMatrix, Color));
}

float3 ApplyGammaCorrectionAndTonemapping(float3 Color)
{
    const float INTENSITY = 1.0f;
    return ApplyGammaInv(AcesFitted(Color));
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