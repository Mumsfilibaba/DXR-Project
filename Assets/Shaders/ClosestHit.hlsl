#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "RayTracingHelpers.hlsli"
#include "RayTracingShading.hlsli"

#if RAY_TRACING_BINDLESS
    #include "BindlessHelpers.hlsli"
    #include "MaterialBindless.hlsli"
#endif

ConstantBuffer<FCamera>                   CameraBuffer   : register(b0);
ConstantBuffer<FRayTracingSceneConstants> SceneConstants : register(b1);

StructuredBuffer<FRayTracingGeometryIndices> GeometryTable  : register(t9);
StructuredBuffer<FMaterial>                  Materials      : register(t8);
TextureCube<float4>                          IBLDiffuse     : register(t5);
TextureCube<float4>                          IBLSpecular    : register(t6);
Texture2D<float2>                            IntegrationLUT : register(t7);

SamplerState EnvironmentSampler : register(s2);
SamplerState LUTSampler         : register(s3);

#if !RAY_TRACING_BINDLESS
    StructuredBuffer<FVertexAttributes> InAttributes    : register(t0, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
    ByteAddressBuffer                   InIndices       : register(t1, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
    Texture2D<float4>                   AlbedoTex       : register(t2, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
    Texture2D<float4>                   NormalTex       : register(t3, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
    Texture2D<float4>                   MaterialTex     : register(t4, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
    SamplerState                        MaterialSampler : register(s0, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
#endif

[shader("closesthit")]
void ClosestHit(inout FRayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
    const FRayTracingGeometryIndices GeometryIndices = GeometryTable[InstanceID()];
    const FMaterial                  MaterialData    = Materials[GeometryIndices.MaterialIndex];

#if RAY_TRACING_BINDLESS
    StructuredBuffer<FVertexAttributes> InAttributes    = GetResourceFromPackedDescriptorIndex(GeometryIndices.AttributesHandle);
    ByteAddressBuffer                   InIndices       = GetResourceFromPackedDescriptorIndex(GeometryIndices.IndicesHandle);
    Texture2D<float4>                   AlbedoTex       = GetResourceFromPackedDescriptorIndex(MaterialData.AlbedoHandle);
    Texture2D<float4>                   NormalTex       = GetResourceFromPackedDescriptorIndex(MaterialData.NormalHandle);
    Texture2D<float4>                   MaterialTex     = GetResourceFromPackedDescriptorIndex(MaterialData.MaterialHandle);
    SamplerState                        MaterialSampler = GetMaterialSamplerBindless(MaterialData);
#endif

    FHitSurface Surface = InterpolateTriangleHit(InAttributes, InIndices, PrimitiveIndex(), IntersectionAttributes.barycentrics);
    TransformHitSurfaceToWorld(Surface, ObjectToWorld3x4(), WorldToObject3x4());

    const float3 FacingViewDir = normalize(-WorldRayDirection());
    const bool   bHasNormalMap = HasNormalMap(MaterialData);
    
    float3 Normal;
    if (bHasNormalMap && length(Surface.Tangent) > 1e-4f)
    {
        const float3 SampledNormal = UnpackNormalBC5(NormalTex.SampleLevel(MaterialSampler, Surface.TexCoord, 0).rgb);
        const float3 MappedNormal  = ApplyNormalMapAxis(SampledNormal, IsNormalMapPositiveY(MaterialData));
        const float  TangentSign   = Surface.TangentSign * GeometryIndices.DeterminantSign;

        Normal = DecodeTangentNormal(MappedNormal, Surface.Normal, Surface.Tangent, TangentSign);
    }
    else
    {
        Normal = Surface.Normal;
    }

    if (dot(Normal, FacingViewDir) < 0.0f)
    {
        Normal = -Normal;
    }

    // TODO: We should have a more proper texture LOD selection here.  
    const float LOD = (min(RayTCurrent(), 1000.0f) / 1000.0f) * 15.0f;

    float3 AlbedoColor    = SRGBToLinear(AlbedoTex.SampleLevel(MaterialSampler, Surface.TexCoord, LOD).rgb);
    float3 MaterialParams = MaterialTex.SampleLevel(MaterialSampler, Surface.TexCoord, 0).rgb; // r=AO, g=Roughness, b=Metallic

    const float3 HitPosition = WorldHitPosition();

    AlbedoColor    *= MaterialData.Albedo;
    MaterialParams *= float3(MaterialData.AO, MaterialData.Roughness, MaterialData.Metallic);

    const float3 ViewDir = normalize(-WorldRayDirection());
    
    PayLoad.Color = ShadeReflectionHit(
        SceneConstants, IBLDiffuse, IBLSpecular, IntegrationLUT, EnvironmentSampler,
        LUTSampler, AlbedoColor, Normal, MaterialParams, HitPosition, ViewDir);

    PayLoad.HitT  = RayTCurrent();
}
