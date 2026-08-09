#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "RayTracingHelpers.hlsli"
#include "RayTracingShading.hlsli"

#if ENABLE_BINDLESS
    #include "BindlessHelpers.hlsli"
#endif

#define MATERIAL_SRV_REGISTER_BASE  2
#define MATERIAL_SRV_REGISTER_SPACE D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL
#define MATERIAL_ARRAY_REGISTER     t8
#include "MaterialSampling.hlsli"

ConstantBuffer<FCamera>                   CameraBuffer   : register(b0);
ConstantBuffer<FRayTracingSceneConstants> SceneConstants : register(b1);

StructuredBuffer<FRayTracingGeometryIndices> GeometryTable  : register(t9);
TextureCube<float4>                          IBLDiffuse     : register(t5);
TextureCube<float4>                          IBLSpecular    : register(t6);
Texture2D<float2>                            IntegrationLUT : register(t7);

SamplerState EnvironmentSampler : register(s2);
SamplerState LUTSampler         : register(s3);

#if !ENABLE_BINDLESS
    StructuredBuffer<FVertexAttributes> InAttributes : register(t0, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
    ByteAddressBuffer                   InIndices    : register(t1, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
#endif

[shader("closesthit")]
void ClosestHit(inout FRayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
    const FRayTracingGeometryIndices GeometryIndices = GeometryTable[InstanceID()];
    const FMaterial                  MaterialData    = Materials[GeometryIndices.MaterialIndex];

#if ENABLE_BINDLESS
    StructuredBuffer<FVertexAttributes> InAttributes = GetResourceFromPackedDescriptorIndex(GeometryIndices.AttributesHandle);
    ByteAddressBuffer                   InIndices    = GetResourceFromPackedDescriptorIndex(GeometryIndices.IndicesHandle);
#endif

    FHitSurface Surface = InterpolateTriangleHit(InAttributes, InIndices, PrimitiveIndex(), IntersectionAttributes.barycentrics);
    TransformHitSurfaceToWorld(Surface, ObjectToWorld3x4(), WorldToObject3x4());

    const float3 FacingViewDir = normalize(-WorldRayDirection());
    const bool   bHasNormalMap = HasNormalMap(MaterialData);
    
    float3 Normal;
    if (bHasNormalMap && length(Surface.Tangent) > 1e-4f)
    {
        const float3 NormalTexel  = SampleMaterialSlotLevel(MaterialData, MATERIAL_SLOT_NORMAL, Surface.TexCoord, 0).rgb;
        const float3 MappedNormal = DecodeMaterialNormalTS(MaterialData, NormalTexel);
        const float  TangentSign  = Surface.TangentSign * GeometryIndices.DeterminantSign;

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

    // Base colour takes the distance-based LOD while the scalars stay at the top mip.
    const float3 AlbedoColor = SRGBToLinear(SampleMaterialSlotLevel(MaterialData, MATERIAL_SLOT_BASE_COLOR, Surface.TexCoord, LOD).rgb) * MaterialData.Albedo;

    const float3 MaterialParams = float3(
        SampleRoutedScalarLevel(MaterialData, MATERIAL_SCALAR_OCCLUSION, Surface.TexCoord, 0, MaterialData.AO),
        SampleRoutedScalarLevel(MaterialData, MATERIAL_SCALAR_ROUGHNESS, Surface.TexCoord, 0, MaterialData.Roughness),
        SampleRoutedScalarLevel(MaterialData, MATERIAL_SCALAR_METALLIC, Surface.TexCoord, 0, MaterialData.Metallic));

    const float3 HitPosition = WorldHitPosition();
    const float3 ViewDir     = normalize(-WorldRayDirection());
    
    PayLoad.Color = ShadeReflectionHit(
        SceneConstants, IBLDiffuse, IBLSpecular, IntegrationLUT, EnvironmentSampler,
        LUTSampler, AlbedoColor, Normal, MaterialParams, HitPosition, ViewDir);

    PayLoad.HitT  = RayTCurrent();
}
