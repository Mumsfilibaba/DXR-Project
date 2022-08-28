#include "Structs.hlsli"
#include "Constants.hlsli"

ConstantBuffer<FCamera> CameraBuffer : register(b0, space0);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// AABB Debug

cbuffer TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 TransformMat;
};


float4 AABB_VSMain(float3 Position : POSITION0) : SV_Position
{
    return mul(mul(float4(Position, 1.0f), TransformMat), CameraBuffer.ViewProjection);
}

float4 AABB_PSMain() : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// PointLight Debug

cbuffer LightInfoBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4 Color;
    float3 WorldPosition;
    float  Padding;
};

float4 Light_VSMain(float3 Position : POSITION0) : SV_Position
{
    float3 Pos = Position + WorldPosition;
    return mul(float4(Pos, 1.0f), CameraBuffer.ViewProjection);
}

float4 Light_PSMain() : SV_Target
{
    return float4(Color.rgb, 1.0f);
}