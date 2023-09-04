#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0);

TextureCube<float4> Skybox        : register(t1);
Texture2D<float4>   GBufferNormal : register(t2);
Texture2D<float4>   GBufferDepth  : register(t3);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SamplerState GBufferSampler : register(s0);

RWTexture2D<float4> OutTexture : register(u0);

[shader("raygeneration")]
void RayGen()
{
    uint3 DispatchIndex      = DispatchRaysIndex();
    uint3 DispatchDimensions = DispatchRaysDimensions();

    float2 TexCoord = float2(DispatchIndex.xy) / float2(DispatchDimensions.xy);

    float Depth = GBufferDepth.SampleLevel(GBufferSampler, TexCoord, 0).r;
    //if (Depth >= 1.0f)
    //{
    //    OutTexture[DispatchIndex.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    //    return;
    //}
    
    float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    float3 WorldNormal   = GBufferNormal.SampleLevel(GBufferSampler, TexCoord, 0).rgb;
    WorldNormal = UnpackNormal(WorldNormal);
    
    float3 ViewDir = normalize(WorldPosition - CameraBuffer.Position);
    
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim   = DispatchRaysDimensions();

    float2 crd  = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd / dims) * 2.f - 1.f);
    
    float3 Forward = normalize(CameraBuffer.Forward);
    float3 Right   = normalize(cross(float3(0.0f, 1.0f, 0.0f), Forward));
    float3 Up      = normalize(-cross(Right, Forward));
    
    float3 Direction = Forward + (Right * (d.x * CameraBuffer.AspectRatio)) + (Up * (-d.y));
    
    RayDesc Ray;
    Ray.Origin    = CameraBuffer.Position; //WorldPosition + (WorldNormal * RAY_OFFSET);
    Ray.Direction = normalize(Direction);  //normalize(reflect(ViewDir, WorldNormal));
    Ray.TMin      = CameraBuffer.NearPlane;
    Ray.TMax      = 10000.0f;

    RayPayload PayLoad;
    PayLoad.CurrentDepth = 1;

    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, PayLoad);

    OutTexture[DispatchIndex.xy] = float4(ApplyGammaCorrectionAndTonemapping(PayLoad.Color), 1.0f);
}