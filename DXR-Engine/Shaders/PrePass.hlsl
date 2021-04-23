#include "Structs.hlsli"
#include "Constants.hlsli"

// PerObject Constants
cbuffer TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 TransformMat;
};

// PerFrame
ConstantBuffer<Camera> CameraBuffer : register(b0);

Texture2D<float4> DiffuseAlphaTex : register(t0);

SamplerState Sampler : register(s0);

// VertexShader
struct VSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION;
};

PSInput VSMain(VSInput Input)
{
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), TransformMat);

    PSInput Output;
    Output.Position = mul(WorldPosition, CameraBuffer.ViewProjection);
    Output.TexCoord = Input.TexCoord;

    return Output;
}

void PSMain(PSInput Input)
{
    float2 TexCoord = Input.TexCoord;
    TexCoord.y = 1.0f - TexCoord.y;
    
    float4 DiffuseAlpha = DiffuseAlphaTex.Sample(Sampler, TexCoord);
    if (DiffuseAlpha.a < 0.5f)
    {
        discard;
    }
}