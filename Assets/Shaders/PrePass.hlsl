#include "Structs.hlsli"
#include "Constants.hlsli"

// PerObject Constants
cbuffer TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 TransformMat;
};

// PerFrame
ConstantBuffer<FCamera> CameraBuffer : register(b0, space0);

// VertexShader
struct FVSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

float4 Main(FVSInput Input) : SV_POSITION
{
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), TransformMat);
    return mul(WorldPosition, CameraBuffer.ViewProjection);
}