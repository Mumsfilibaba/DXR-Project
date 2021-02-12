#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"

TextureCube<float4> Skybox  : register(t1, space0);
SamplerState TextureSampler : register(s0, space0);

[shader("miss")]
void Miss(inout RayPayload PayLoad)
{
    PayLoad.Color = Skybox.SampleLevel(TextureSampler, WorldRayDirection(), 0).rgb; // float3(0.3921f, 0.5843f, 0.9394f);
}
