#ifndef HELPERS_HLSLI
#define HELPERS_HLSLI

#include "Constants.hlsli"
#include "DepthHelpers.hlsli"

// Float helpers

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

// Mapping Helpers

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

// Luma

float Luma(float3 Color)
{
    return sqrt(dot(Color, float3(0.2126f, 0.587f, 0.114f)));
}

float Luminance(float3 Color)
{
    return dot(Color, float3(0.2126f, 0.7152f, 0.0722f));
}

// Plane helpers

float4 CreatePlane(float3 Q, float3 R)
{
    float3 N = normalize(cross(Q, R));
    return float4(N, 0);
}

float GetSignedDistanceFromPlane(float3 P, float4 Plane)
{
    return dot(Plane.xyz, P);
}

float4 PlaneFromPoints(in float3 Point1, in float3 Point2, in float3 Point3)
{
    float3 v21 = Point1 - Point2;
    float3 v31 = Point1 - Point3;

    float3 Normal = normalize(cross(v21, v31));
    float  Offset = -dot(Normal, Point1);

    return float4(Normal, Offset);
}

// Math Helpers

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

// Normal-Mapping Helpers

float3 ApplyNormalMapping(float3 MappedNormal, float3 Normal, float3 Tangent, float3 Bitangent)
{
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    return normalize(mul(MappedNormal, TBN));
}

min16float3 ApplyNormalMapping(min16float3 MappedNormal, min16float3 Normal, min16float3 Tangent, min16float3 Bitangent)
{
    min16float3x3 TBN = min16float3x3(Tangent, Bitangent, Normal);
    return normalize(mul(MappedNormal, TBN));
}

float3 UnpackNormal(float3 TextureSample)
{
    return normalize((TextureSample * 2.0f) - 1.0f);
}

min16float3 UnpackNormal(min16float3 TextureSample)
{
    return normalize((TextureSample * 2.0) - 1.0);
}

float3 UnpackNormalBC5(float3 TextureSample)
{
	float2 NormalXY = TextureSample.rg;	
	NormalXY = (NormalXY * 2.0f) - 1.0f;
	float NormalZ = sqrt(saturate(1.0f - dot(NormalXY, NormalXY)));
	return float3(NormalXY.xy, NormalZ);
}

min16float3 UnpackNormalBC5(min16float3 TextureSample)
{
	min16float2 NormalXY = TextureSample.rg;	
	NormalXY = (NormalXY * 2.0) - 1.0;
	min16float NormalZ = sqrt(saturate(1.0 - dot(NormalXY, NormalXY)));
	return min16float3(NormalXY.xy, NormalZ);
}

float3 PackNormal(float3 Normal)
{
    return (normalize(Normal) + 1.0f) * 0.5f;
}

min16float3 PackNormal(min16float3 Normal)
{
    return (normalize(Normal) + 1.0) * 0.5;
}

// ClipAABB

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