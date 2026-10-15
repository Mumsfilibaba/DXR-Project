#ifndef RAY_TRACING_ALPHA_TEST_HLSLI
#define RAY_TRACING_ALPHA_TEST_HLSLI

// Needs MaterialSampling.hlsli with slot access, and RayTracingShading.hlsli

// The base pass's cutoff, so a masked surface is cut out the same way when a ray hits it
#define RAY_TRACING_ALPHA_CUTOFF (0.5f)

bool PassesAlphaTest(FMaterial Material, StructuredBuffer<FVertexAttributes> InAttributes, ByteAddressBuffer InIndices, uint PrimitiveIndex, float2 Barycentrics)
{
    const float2 TexCoord = InterpolateTriangleTexCoord(InAttributes, InIndices, PrimitiveIndex, Barycentrics);
    return SampleRoutedScalarLevel(Material, MATERIAL_SCALAR_OPACITY, TexCoord, 0.0f, Material.Opacity) >= RAY_TRACING_ALPHA_CUTOFF;
}

#if STATS_ENABLED

void CountAlphaTests(RWStructuredBuffer<uint> Counter)
{
    const uint NumTests = WaveActiveCountBits(true);
    if (WaveIsFirstLane())
    {
        InterlockedAdd(Counter[0], NumTests);
    }
}

#endif

#if MATERIAL_SLOT_ACCESS == MATERIAL_SLOT_ACCESS_BINDLESS

bool PassesAlphaTestBindless(FRayTracingGeometryIndices GeometryIndices, FMaterial Material, uint PrimitiveIndex, float2 Barycentrics)
{
    StructuredBuffer<FVertexAttributes> InAttributes = GetResourceFromPackedDescriptorIndexNonUniform(GeometryIndices.AttributesHandle);
    ByteAddressBuffer                   InIndices    = GetResourceFromPackedDescriptorIndexNonUniform(GeometryIndices.IndicesHandle);
    return PassesAlphaTest(Material, InAttributes, InIndices, GeometryIndices.FirstTriangle + PrimitiveIndex, Barycentrics);
}

#endif

#endif
