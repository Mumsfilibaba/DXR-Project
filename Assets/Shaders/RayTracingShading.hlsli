#ifndef RAY_TRACING_SHADING_HLSLI
#define RAY_TRACING_SHADING_HLSLI

#include "Structs.hlsli"
#include "PBRHelpers.hlsli"
#include "ImageBasedLighting.hlsli"
#include "RayTracingBindless.hlsli"
#include "TangentSpace.hlsli"
#include "VertexPacking.hlsli"

struct FHitSurface
{
    float3 Normal;  
    float3 Tangent; 
    float  TangentSign;
    float2 TexCoord;
};

FHitSurface InterpolateTriangleHit(StructuredBuffer<FVertexAttributes> InAttributes, ByteAddressBuffer InIndices, uint PrimitiveIndex, float2 Barycentrics)
{
    const uint   TriangleIndexStride = 3 * 4;
    const uint3  Indices             = InIndices.Load3(PrimitiveIndex * TriangleIndexStride);
    const float3 BarycentricCoords   = float3(1.0f - Barycentrics.x - Barycentrics.y, Barycentrics.x, Barycentrics.y);

    const FVertexAttributes Attributes0 = InAttributes[Indices[0]];
    const FVertexAttributes Attributes1 = InAttributes[Indices[1]];
    const FVertexAttributes Attributes2 = InAttributes[Indices[2]];

    const float4 Tangent0 = UnpackSnorm16x4(Attributes0.PackedTangent);

    FHitSurface Surface;
    Surface.Normal = normalize(
        (UnpackSnorm16x4(Attributes0.PackedNormal).xyz * BarycentricCoords.x) +
        (UnpackSnorm16x4(Attributes1.PackedNormal).xyz * BarycentricCoords.y) +
        (UnpackSnorm16x4(Attributes2.PackedNormal).xyz * BarycentricCoords.z));

    Surface.Tangent = normalize(
        (Tangent0.xyz                                   * BarycentricCoords.x) +
        (UnpackSnorm16x4(Attributes1.PackedTangent).xyz * BarycentricCoords.y) +
        (UnpackSnorm16x4(Attributes2.PackedTangent).xyz * BarycentricCoords.z));

    Surface.TangentSign = Tangent0.w;

    Surface.TexCoord =
        (Attributes0.TexCoord * BarycentricCoords.x) +
        (Attributes1.TexCoord * BarycentricCoords.y) +
        (Attributes2.TexCoord * BarycentricCoords.z);

    return Surface;
}

void TransformHitSurfaceToWorld(inout FHitSurface Surface, float3x4 ObjectToWorld, float3x4 WorldToObject)
{
    Surface.Normal  = normalize(mul(Surface.Normal, (float3x3)WorldToObject));
    Surface.Tangent = normalize(mul((float3x3)ObjectToWorld, Surface.Tangent));
}

float3 ShadeReflectionHit(FRayTracingSceneConstants Lighting, TextureCube<float4> DiffuseCube, TextureCube<float4> SpecularCube, Texture2D<float2> IntegrationLUT,
    SamplerState EnvironmentSampler, SamplerState LUTSampler, float3 AlbedoColor, float3 Normal, float3 MaterialParams, float3 HitPosition, float3 ViewDir)
{
    const float SampledAO        = MaterialParams.r;
    const float SampledRoughness = MaterialParams.g;
    const float SampledMetallic  = MaterialParams.b;

    const float3 N  = Normal;
    const float3 V  = ViewDir;
    const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), AlbedoColor, SampledMetallic);

    // -------------------------------------------------------------------------------------
    // Direct lighting
    // -------------------------------------------------------------------------------------

    float3 L0 = 0.0f;

    // Directional sun
    
    {
        const float3 L        = normalize(-Lighting.SunDirection);
        const float3 Radiance = Lighting.SunColor;
        L0 += DirectRadiance(F0, N, V, L, Radiance, AlbedoColor, SampledRoughness, SampledMetallic);
    }

    // Scene point lights

    [loop]
    for (uint i = 0; i < Lighting.NumPointLights; ++i)
    {
        const float3 LightPosition = Lighting.PointLightPositionRadius[i].xyz;
        const float3 LightColor    = Lighting.PointLightColor[i].rgb;

        float3 L            = LightPosition - HitPosition;
        float  DistanceSqrd = dot(L, L);
        float  Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
        L = normalize(L);

        const float3 Radiance = LightColor * Attenuation;
        L0 += DirectRadiance(F0, N, V, L, Radiance, AlbedoColor, SampledRoughness, SampledMetallic);
    }

    // -------------------------------------------------------------------------------------
    // Indirect lighting
    // -------------------------------------------------------------------------------------

    const float  NDotV      = max(dot(N, V), 0.0f);
    const float3 Reflection = reflect(-V, N);

    const float3 F  = FresnelSchlick_Roughness(F0, V, N, SampledRoughness);
    const float3 Ks = F;
    const float3 Kd = 1.0f - Ks;

    FDiffuseEnvironmentInfo DiffuseInfo;
    DiffuseInfo.NormalUVW = N;

    FSpecularEnvironmentInfo SpecularInfo;
    SpecularInfo.ReflectionUVW = Reflection;
    SpecularInfo.Roughness     = SampledRoughness;

    const float3 DiffuseSample   = DiffuseEnvironment(DiffuseCube, EnvironmentSampler, DiffuseInfo);
    const float3 SpecularSample  = SpecularEnvironment(SpecularCube, EnvironmentSampler, SpecularInfo, Lighting.NumSkyLightMips);
    const float2 BRDFIntegration = GetIntegrationConstants(IntegrationLUT, LUTSampler, NDotV, SampledRoughness);

    const float3 Specular = SpecularSample * (Ks * BRDFIntegration.x + BRDFIntegration.y);
    const float3 Diffuse  = DiffuseSample * AlbedoColor * Kd;
    const float3 Ambient  = (Diffuse + Specular) * SampledAO;

    return Ambient + L0;
}

#endif
