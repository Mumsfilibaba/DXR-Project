#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "Random.hlsli"
#include "RayTracingHelpers.hlsli"
#include "RayTracingShading.hlsli"
#include "BindlessHelpers.hlsli"
#include "MaterialBindless.hlsli"

ConstantBuffer<FCamera>                     CameraBuffer   : register(b0);
ConstantBuffer<FRayTracingSceneConstants>   SceneConstants : register(b1);

RaytracingAccelerationStructure              Scene           : register(t0);
TextureCube<float4>                          Skybox          : register(t1);
Texture2D<float4>                            GBufferNormal   : register(t2);
Texture2D<float4>                            GBufferDepth    : register(t3);
Texture2D<float4>                            GBufferMaterial : register(t4);
TextureCube<float4>                          IBLDiffuse      : register(t5);
TextureCube<float4>                          IBLSpecular     : register(t6);
Texture2D<float2>                            IntegrationLUT  : register(t7);
StructuredBuffer<FMaterial>                  Materials       : register(t8);
StructuredBuffer<FRayTracingGeometryIndices> GeometryTable   : register(t9);
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4>   OutTexture      : register(u0);

SamplerState GBufferSampler     : register(s0);
SamplerState EnvironmentSampler : register(s2);
SamplerState LUTSampler         : register(s3);

[numthreads(8, 8, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint OutputWidth;
    uint OutputHeight;
    OutTexture.GetDimensions(OutputWidth, OutputHeight);

    const uint2 Pixel = DispatchThreadID.xy;
    if (Pixel.x >= OutputWidth || Pixel.y >= OutputHeight)
    {
        return;
    }

    const float2 TexCoord = (float2(Pixel) + 0.5f) / float2(OutputWidth, OutputHeight);
    const float  Depth    = GBufferDepth.SampleLevel(GBufferSampler, TexCoord, 0).r;

    if (Depth >= 1.0f)
    {
        OutTexture[Pixel] = float4(0.0f, 0.0f, 0.0f, -1.0f);
        return;
    }

    const float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    const float3 WorldNormal   = UnpackNormal(GBufferNormal.SampleLevel(GBufferSampler, TexCoord, 0).rgb);
    const float  Roughness     = saturate(GBufferMaterial.SampleLevel(GBufferSampler, TexCoord, 0).r);
    const float3 ViewDir       = normalize(WorldPosition - CameraBuffer.PositionWS);

    float3 ReflectDirection;
    if (Roughness < RAY_TRACING_MIRROR_ROUGHNESS_THRESHOLD)
    {
        ReflectDirection = normalize(reflect(ViewDir, WorldNormal));
    }
    else
    {
        uint   Seed = InitRandom(Pixel, OutputWidth, SceneConstants.FrameIndex);
        float2 Xi   = NextRandom2(Seed);
        float3 H    = ImportanceSampleGGX(Xi, Roughness, WorldNormal);
        ReflectDirection = normalize(reflect(ViewDir, H));
    }

    RayDesc Ray;
    Ray.Origin    = WorldPosition + (WorldNormal * RAY_OFFSET);
    Ray.Direction = ReflectDirection;
    Ray.TMin      = 0.0f;
    Ray.TMax      = 10000.0f;

    RayQuery<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> Query;
    Query.TraceRayInline(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, Ray);
    Query.Proceed();

    float3 Color;
    float  OutHitT = -1.0f;

    if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        const float HitT = Query.CommittedRayT();
        OutHitT = HitT;

        const FRayTracingGeometryIndices GeometryIndices = GeometryTable[Query.CommittedInstanceID()];
        const FMaterial                  MaterialData    = Materials[GeometryIndices.MaterialIndex];

        StructuredBuffer<FVertex> InVertices      = GetResourceFromPackedDescriptorIndexNonUniform(GeometryIndices.VerticesHandle);
        ByteAddressBuffer         InIndices       = GetResourceFromPackedDescriptorIndexNonUniform(GeometryIndices.IndicesHandle);
        Texture2D<float4>         AlbedoTex       = GetResourceFromPackedDescriptorIndexNonUniform(MaterialData.AlbedoHandle);
        Texture2D<float4>         NormalTex       = GetResourceFromPackedDescriptorIndexNonUniform(MaterialData.NormalHandle);
        Texture2D<float4>         MaterialTex     = GetResourceFromPackedDescriptorIndexNonUniform(MaterialData.MaterialHandle);
        SamplerState              MaterialSampler = GetMaterialSamplerBindless(MaterialData);

        FHitSurface Surface = InterpolateTriangleHit(InVertices, InIndices, Query.CommittedPrimitiveIndex(), Query.CommittedTriangleBarycentrics());
        TransformHitSurfaceToWorld(Surface, Query.CommittedObjectToWorld3x4(), Query.CommittedWorldToObject3x4());

        const float3 HitViewDir = normalize(-Ray.Direction);

        float3 Normal;
        if (MaterialData.NormalMapFlags != 0 && length(Surface.Tangent) > 1e-4f)
        {
            const float3 MappedNormal = UnpackNormalBC5(NormalTex.SampleLevel(MaterialSampler, Surface.TexCoord, 0).rgb);
            const float3 Bitangent    = normalize(cross(Surface.Normal, Surface.Tangent));
            Normal = ApplyNormalMapping(MappedNormal, Surface.Normal, Surface.Tangent, Bitangent);
        }
        else
        {
            Normal = Surface.Normal;
        }

        if (dot(Normal, HitViewDir) < 0.0f)
        {
            Normal = -Normal;
        }

        const float  LOD         = (min(HitT, 1000.0f) / 1000.0f) * 15.0f;
        const float3 AlbedoColor = SRGBToLinear(AlbedoTex.SampleLevel(MaterialSampler, Surface.TexCoord, LOD).rgb) * MaterialData.Albedo;
        const float3 HitPosition = Ray.Origin + (Ray.Direction * HitT);
        
        float3 MaterialParams = MaterialTex.SampleLevel(MaterialSampler, Surface.TexCoord, 0).rgb; // r=AO, g=Roughness, b=Metallic
        MaterialParams *= float3(MaterialData.AO, MaterialData.Roughness, MaterialData.Metallic);

        Color = ShadeReflectionHit(SceneConstants, IBLDiffuse, IBLSpecular, IntegrationLUT, EnvironmentSampler, 
            LUTSampler, AlbedoColor, Normal, MaterialParams, HitPosition, HitViewDir);
    }
    else
    {
        Color = Skybox.SampleLevel(GBufferSampler, ReflectDirection, 0).rgb;
    }

    OutTexture[Pixel] = float4(Color, OutHitT);
}
