#ifndef HELPERS_HLSLI
#define HELPERS_HLSLI

#include "Constants.hlsli"

float2 Float2(float Scalar)
{
    return float2(Scalar, Scalar);
}

float3 Float3(float Scalar)
{
    return float3(Scalar, Scalar, Scalar);
}

float4 Float4(float Scalar)
{
    return float4(Scalar, Scalar, Scalar, Scalar);
}

float MinusOneToOne(float v)
{
    return v * 0.5f + 0.5f;
}

float2 MinusOneToOne(float2 v)
{
    return v * 0.5f + 0.5f;
}

float3 MinusOneToOne(float3 v)
{
    return v * 0.5f + 0.5f;
}

float OneToMinusOne(float v)
{
    return v * 2.0f - 1.0f;
}

float2 OneToMinusOne(float2 v)
{
    return v * 2.0f - 1.0f;
}

float3 OneToMinusOne(float3 v)
{
    return v * 2.0f - 1.0f;
}

float Luma(float3 Color)
{
    return sqrt(dot(Color, float3(0.2126f, 0.587f, 0.114f)));
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

float Random(float3 Seed, int i)
{
    float4 Seed4 = float4(Seed, i);
    float  Dot   = dot(Seed4, float4(12.9898f, 78.233f, 45.164f, 94.673f));
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

    float4 ProjectedPos  = float4(x, y, z, 1.0f);
    
    float4 FinalPosition = mul(ProjectedPos, ProjectionInverse);  
    return FinalPosition.xyz / FinalPosition.w;
}

float DepthClipToEye(float Near, float Far, float z)
{
    return Near + (Far - Near) * z;
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

// Modified version from: https://github.com/playdeadgames/temporal/blob/master/Assets/Shaders/TemporalReprojection.shader
float3 ClipAABB(float3 MinAABB, float3 MaxAABB, float3 Q)
{
    // NOTE: Only clips towards AABB center (but fast!)
    float3 ClipO = 0.5f * (MaxAABB + MinAABB);
    float3 ClipE = 0.5f * (MaxAABB - MinAABB) + FLT32_EPSILON;

    float3 ClipV  = Q - ClipO;
    float3 UnitV  = ClipV / ClipE;
    float3 UnitA  = abs(UnitV);
    float  UnitMa = max(UnitA.x, max(UnitA.y, UnitA.z));

    if (UnitMa > 1.0f)
    {
        return ClipO + (ClipV / UnitMa);
    }
    else
    {    
        // Point inside AABB
        return Q;
    }
}

#endif