#include "Structs.hlsli"
#include "Constants.hlsli"
#include "DepthHelpers.hlsli"
#include "DebugColor.hlsli"

ConstantBuffer<FCamera> CameraBuffer : register(b0);

RaytracingAccelerationStructure            Scene      : register(t0);
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> OutTexture : register(u0);

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
    Query.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, Ray);
    Query.Proceed();

    float3 Color;
    if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        Color = InstanceIDToColor(Query.CommittedInstanceID());
    }
    else
    {
        Color = float3(0.0f, 0.0f, 0.0f);
    }

    OutTexture[Pixel] = float4(Color, 1.0f);
}
