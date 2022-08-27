#include "Helpers.hlsli"
#include "Constants.hlsli"

// Resources
cbuffer CameraBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 ViewProjection;
};

TextureCube<float4> Skybox        : register(t0, space0);
SamplerState        SkyboxSampler : register(s0, space0);

// VertexShader
struct FVSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct FVSOutput
{
    float3 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION0;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;
    Output.TexCoord = Input.Position;
    Output.Position = mul(float4(Input.Position, 1.0f), ViewProjection);
    Output.Position = Output.Position.xyww;
    return Output;
}

// PixelShader
float4 PSMain(float3 TexCoord : TEXCOORD0) : SV_TARGET0
{
    float3 Color = Skybox.Sample(SkyboxSampler, TexCoord).rgb;
    // Finalize
    float FinalLuminance = Luminance(Color);
    Color = ApplyGammaCorrectionAndTonemapping(Color);
    return float4(Color, FinalLuminance);
}