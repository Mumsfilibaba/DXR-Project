#include "Structs.hlsli"

// PerObject Constants
cbuffer TransformBuffer : register(b0, space0)
{
    float4x4 TransformMat;
};

// PerFrame
ConstantBuffer<Camera> CameraBuffer : register(b1, space0);

// VertexShader
struct VSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

float4 Main(VSInput Input) : SV_POSITION
{
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), TransformMat);
    return mul(WorldPosition, CameraBuffer.ViewProjection);
}