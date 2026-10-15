#include "Structs.hlsli"
#include "Constants.hlsli"
#include "RayTracingHelpers.hlsli"
#include "RayTracingShading.hlsli"

#if ENABLE_BINDLESS
    #include "BindlessHelpers.hlsli"
#endif

#define MATERIAL_SRV_REGISTER_BASE  2
#define MATERIAL_SRV_REGISTER_SPACE D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL
#define MATERIAL_ARRAY_REGISTER     t8
#include "MaterialSampling.hlsli"
#include "RayTracingAlphaTest.hlsli"

StructuredBuffer<FRayTracingGeometryIndices> GeometryTable : register(t9);

#if STATS_ENABLED
    RWStructuredBuffer<uint> AlphaTests : register(u1);
#endif

#if !ENABLE_BINDLESS
    StructuredBuffer<FVertexAttributes> InAttributes : register(t0, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
    ByteAddressBuffer                   InIndices    : register(t1, D3D12_SHADER_REGISTER_SPACE_RAY_TRACING_LOCAL);
#endif

[shader("anyhit")]
void AnyHit(inout FRayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
    const FRayTracingGeometryIndices GeometryIndices = GeometryTable[InstanceID() + GeometryIndex()];
    const FMaterial                  MaterialData    = Materials[GeometryIndices.MaterialIndex];

#if ENABLE_BINDLESS
    StructuredBuffer<FVertexAttributes> InAttributes = GetResourceFromPackedDescriptorIndex(GeometryIndices.AttributesHandle);
    ByteAddressBuffer                   InIndices    = GetResourceFromPackedDescriptorIndex(GeometryIndices.IndicesHandle);
#endif

#if STATS_ENABLED
    CountAlphaTests(AlphaTests);
#endif

    if (!PassesAlphaTest(MaterialData, InAttributes, InIndices, GeometryIndices.FirstTriangle + PrimitiveIndex(), IntersectionAttributes.barycentrics))
    {
        IgnoreHit();
    }
}
