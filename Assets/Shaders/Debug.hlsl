#include "Structs.hlsli"
#include "Constants.hlsli"

cbuffer TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 TransformMat;
};

ConstantBuffer<FCamera> CameraBuffer : register(b0, space0);

float4 VSMain(float3 Position : POSITION0) : SV_Position
{
    return mul(mul(float4(Position, 1.0f), TransformMat), CameraBuffer.ViewProjection);
}

float4 PSMain() : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}