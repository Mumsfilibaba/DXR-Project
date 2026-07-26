#ifndef HELPERS_HLSLI
#define HELPERS_HLSLI

#include "Constants.hlsli"
#include "DepthHelpers.hlsli"

// ------------------------------------------------------------------------------------------------
// Mapping Helpers
// ------------------------------------------------------------------------------------------------

float MinusOneToOne(float v)
{
    return v * 0.5 + 0.5;
}

float2 MinusOneToOne(float2 v)
{
    return v * 0.5 + 0.5;
}

float3 MinusOneToOne(float3 v)
{
    return v * 0.5 + 0.5;
}

float OneToMinusOne(float v)
{
    return v * 2.0 - 1.0;
}

float2 OneToMinusOne(float2 v)
{
    return v * 2.0 - 1.0;
}

float3 OneToMinusOne(float3 v)
{
    return v * 2.0 - 1.0;
}

// ------------------------------------------------------------------------------------------------
// Luma
// ------------------------------------------------------------------------------------------------

float Luma(float3 Color)
{
    return sqrt(dot(Color, float3(0.2126f, 0.587f, 0.114f)));
}

float Luminance(float3 Color)
{
    return dot(Color, float3(0.2126f, 0.7152f, 0.0722f));
}

// ------------------------------------------------------------------------------------------------
// Plane helpers
// ------------------------------------------------------------------------------------------------

float4 CreatePlane(float3 EdgeA, float3 EdgeB)
{
    float3 PlaneNormal = normalize(cross(EdgeA, EdgeB));
    return float4(PlaneNormal, 0);
}

float GetSignedDistanceFromPlane(float3 Point, float4 Plane)
{
    return dot(Plane.xyz, Point);
}

float4 PlaneFromPoints(in float3 Point1, in float3 Point2, in float3 Point3)
{
    float3 Edge1 = Point1 - Point2;
    float3 Edge2 = Point1 - Point3;

    float3 Normal = normalize(cross(Edge1, Edge2));
    float  Offset = -dot(Normal, Point1);

    return float4(Normal, Offset);
}

// ------------------------------------------------------------------------------------------------
// Math Helpers
// ------------------------------------------------------------------------------------------------

uint DivideByMultiple(uint Value, uint Alignment)
{
    return ((Value + Alignment - 1) / Alignment);
}

float Random(float3 Seed, int Index)
{
    float4 Seed4 = float4(Seed, Index);
    float  DotProduct = dot(Seed4, float4(12.9898f, 78.233f, 45.164f, 94.673f));
    return frac(sin(DotProduct) * 43758.5453f);
}

float Linstep(float Low, float High, float Value)
{
    return saturate((Value - Low) / (High - Low));
}

float Lerp(float Start, float End, float Factor)
{
    return (-Factor * End) + ((Start * Factor) + End);
}

float3 Lerp(float3 Start, float3 End, float Factor)
{
    return (-Factor * End) + ((Start * Factor) + End);
}

// ------------------------------------------------------------------------------------------------
// Normal-Mapping Helpers
// ------------------------------------------------------------------------------------------------

float3 ApplyNormalMapping(float3 TangantNormal, float3 Normal, float3 Tangent, float3 Bitangent)
{
    float3x3 TangentSpace = float3x3(Tangent, Bitangent, Normal);
    return normalize(mul(TangantNormal, TangentSpace));
}

#if MIN16FLOAT_AVAILABLE
min16float3 ApplyNormalMapping(min16float3 TangantNormal, min16float3 Normal, min16float3 Tangent, min16float3 Bitangent)
{
    min16float3x3 TangentSpace = min16float3x3(Tangent, Bitangent, Normal);
    return normalize(mul(TangantNormal, TangentSpace));
}
#endif

float3 UnpackNormal(float3 TextureSample)
{
    return normalize((TextureSample * 2.0) - 1.0);
}

#if MIN16FLOAT_AVAILABLE
min16float3 UnpackNormal(min16float3 TextureSample)
{
    return normalize((TextureSample * 2.0) - 1.0);
}
#endif

float3 UnpackNormalBC5(float3 TextureSample)
{
	float2 NormalXY = TextureSample.rg;	
	NormalXY = (NormalXY * 2.0) - 1.0;
	float NormalZ = sqrt(saturate(1.0 - dot(NormalXY, NormalXY)));
	return float3(NormalXY.xy, NormalZ);
}

#if MIN16FLOAT_AVAILABLE
min16float3 UnpackNormalBC5(min16float3 TextureSample)
{
	min16float2 NormalXY = TextureSample.rg;	
	NormalXY = (NormalXY * 2.0) - 1.0;
	min16float NormalZ = sqrt(saturate(1.0 - dot(NormalXY, NormalXY)));
	return min16float3(NormalXY.xy, NormalZ);
}
#endif

float3 PackNormal(float3 Normal)
{
    return (normalize(Normal) + 1.0) * 0.5;
}

#if MIN16FLOAT_AVAILABLE
min16float3 PackNormal(min16float3 Normal)
{
    return (normalize(Normal) + 1.0) * 0.5;
}
#endif

// ------------------------------------------------------------------------------------------------
// ClipAABB
// ------------------------------------------------------------------------------------------------

// Modified version from: https://github.com/playdeadgames/temporal/blob/master/Assets/Shaders/TemporalReprojection.shader
float3 ClipAABB(float3 MinAABB, float3 MaxAABB, float3 Point)
{
    // NOTE: Only clips towards AABB center (but fast!)
    float3 Center  = 0.5 * (MaxAABB + MinAABB);
    float3 Extent  = 0.5 * (MaxAABB - MinAABB) + FLT32_EPSILON;
    float3 Offset  = Point - Center;
    float3 Unit    = Offset / Extent;
    float3 AbsUnit = abs(Unit);
    float  MaxUnit = max(AbsUnit.x, max(AbsUnit.y, AbsUnit.z));

    if (MaxUnit > 1.0)
    {
        return Center + (Offset / MaxUnit);
    }
    else
    {    
        // Point inside AABB
        return Point;
    }
}

#endif