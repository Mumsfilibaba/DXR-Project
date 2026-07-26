#include "Helpers.hlsli"
#include "Constants.hlsli"
#include "Tonemapping.hlsli"

// Resources
SHADER_CONSTANT_BLOCK_BEGIN
    // 0-64
    float4x4 ViewProjection;
SHADER_CONSTANT_BLOCK_END

TextureCube<float4> Skybox        : register(t0);
SamplerState        SkyboxSampler : register(s0);

// ------------------------------------------------------------------------------------------------
// VertexShader
// ------------------------------------------------------------------------------------------------

struct FVSInput
{
    float3 Position : POSITION0;
};

struct FVSOutput
{
    float3 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION0;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;
    Output.TexCoord = normalize(Input.Position);
    Output.Position = mul(float4(Input.Position, 1.0), Constants.ViewProjection);
    Output.Position = Output.Position.xyww;
    return Output;
}

// ------------------------------------------------------------------------------------------------
// PixelShader
// ------------------------------------------------------------------------------------------------

float4 PSMain(float3 TexCoord : TEXCOORD0) : SV_TARGET0
{
    float3 SkyboxColor    = Skybox.Sample(SkyboxSampler, normalize(TexCoord)).rgb;
    float  FinalLuminance = Luminance(SkyboxColor); // Store Luminance since FXAA assumes it to be in this channel
    return float4(SkyboxColor, FinalLuminance);
}