#include "Structs.hlsli"
#include "Constants.hlsli"
#include "DepthHelpers.hlsli"
#include "DebugColor.hlsli"
#include "RayTracingHelpers.hlsli"
#include "RayTracingBindless.hlsli"
#include "RayTracingShading.hlsli"
#include "BindlessHelpers.hlsli"

#define MATERIAL_SLOT_ACCESS      MATERIAL_SLOT_ACCESS_BINDLESS
#define MATERIAL_SLOT_NON_UNIFORM (1)
#define MATERIAL_ARRAY_REGISTER   t8
#include "MaterialSampling.hlsli"
#include "RayTracingAlphaTest.hlsli"

ConstantBuffer<FCamera> CameraBuffer : register(b0);

RaytracingAccelerationStructure              Scene         : register(t0);
StructuredBuffer<FRayTracingGeometryIndices> GeometryTable : register(t9);
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4>   OutTexture    : register(u0);

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
    const float3 NearWS   = PositionFromDepth(0.0f, TexCoord, CameraBuffer.ViewProjectionInv);
    const float3 FarWS    = PositionFromDepth(1.0f, TexCoord, CameraBuffer.ViewProjectionInv);

    RayDesc Ray;
    Ray.Origin    = NearWS;
    Ray.Direction = normalize(FarWS - NearWS);
    Ray.TMin      = 0.0f;
    Ray.TMax      = 100000.0f;

    RayQuery<RAY_FLAG_NONE> Query;
    Query.TraceRayInline(Scene, RAY_FLAG_NONE, RAY_TRACING_INSTANCE_MASK_ALL, Ray);

    while (Query.Proceed())
    {
        if (Query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            const FRayTracingGeometryIndices CandidateIndices = GeometryTable[Query.CandidateInstanceID() + Query.CandidateGeometryIndex()];
            if ((CandidateIndices.Flags & RT_GEOMETRY_FLAG_ALPHA_TESTED) == 0 ||
                PassesAlphaTestBindless(CandidateIndices, Materials[CandidateIndices.MaterialIndex], Query.CandidatePrimitiveIndex(), Query.CandidateTriangleBarycentrics()))
            {
                Query.CommitNonOpaqueTriangleHit();
            }
        }
    }

    float3 Color;
    if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        Color = InstanceIDToColor(Query.CommittedInstanceID() + Query.CommittedGeometryIndex());
    }
    else
    {
        Color = float3(0.0f, 0.0f, 0.0f);
    }

    OutTexture[Pixel] = float4(Color, 1.0f);
}
