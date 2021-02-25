#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0, space0);

TextureCube<float4> Skybox        : register(t1, space0);
Texture2D<float4>   GBufferNormal : register(t2, space0);
Texture2D<float4>   GBufferDepth  : register(t3, space0);

ConstantBuffer<Camera> CameraBuffer : register(b0, space0);

SamplerState GBufferSampler : register(s0, space0);

RWTexture2D<float4> OutTexture 	: register(u0, space0);

static const float3 LightPosition = float3(0.0f, 1.0f, 0.0f);
static const float3 LightColor    = float3(1.0f, 1.0f, 1.0f);

float2 GetTexCoord()
{
    uint3 DispatchIndex      = DispatchRaysIndex();
    uint3 DispatchDimensions = DispatchRaysDimensions();
    return float2(DispatchIndex.xy) / float2(DispatchDimensions.xy);
}

[shader("raygeneration")]
void RayGen()
{
    uint3 DispatchIndex      = DispatchRaysIndex();
    uint3 DispatchDimensions = DispatchRaysDimensions();

    float2 TexCoord = GetTexCoord();

    // Sample Normal and Position
    float Depth = GBufferDepth.SampleLevel(GBufferSampler, TexCoord, 0).r;
    if (Depth >= 1.0f)
    {
        OutTexture[DispatchIndex.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }
    
    float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInverse);
    float3 WorldNormal   = GBufferNormal.SampleLevel(GBufferSampler, TexCoord, 0).rgb;
    WorldNormal = UnpackNormal(WorldNormal);
    
    float3 ViewDir = normalize(WorldPosition - CameraBuffer.Position);
    
    // Send inital ray
    RayDesc Ray;
    Ray.Origin    = WorldPosition + (WorldNormal * RAY_OFFSET);
    Ray.Direction = reflect(ViewDir, WorldNormal);

    Ray.TMin = 0;
    Ray.TMax = 100000;

    RayPayload PayLoad;
    PayLoad.CurrentDepth = 1;

    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, PayLoad);

    // TODO: Remove this shit, why tonemap here?????????
    // Output Image
    OutTexture[DispatchIndex.xy] = float4(ApplyGammaCorrectionAndTonemapping(PayLoad.Color), 1.0f);
}