#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"

ConstantBuffer<FCamera> CameraBuffer : register(b0);

TextureCube<float4> Skybox             : register(t1);
SamplerState        EnvironmentSampler : register(s2);

[shader("miss")]
void Miss(inout FRayPayload PayLoad)
{
    PayLoad.Color = Skybox.SampleLevel(EnvironmentSampler, WorldRayDirection(), 0).rgb;
    PayLoad.HitT  = -1.0f;
}
