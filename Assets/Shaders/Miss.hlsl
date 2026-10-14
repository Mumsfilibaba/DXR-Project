#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "RayTracingBindless.hlsli"
#include "ImageBasedLighting.hlsli"

ConstantBuffer<FCamera>                   CameraBuffer   : register(b0);
ConstantBuffer<FRayTracingSceneConstants> SceneConstants : register(b1);

TextureCube<float4> Skybox             : register(t1);
SamplerState        EnvironmentSampler : register(s2);

[shader("miss")]
void Miss(inout FRayPayload PayLoad)
{
    PayLoad.Color = Skybox.SampleLevel(EnvironmentSampler, RotateEnvironmentDirection(WorldRayDirection(), SceneConstants.SkyboxRotation), 0).rgb * SceneConstants.SkyboxIntensity;
    PayLoad.HitT  = -1.0f;
}
