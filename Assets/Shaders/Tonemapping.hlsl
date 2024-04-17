#include "Constants.hlsli"
#include "Tonemapping.hlsli"

Texture2D    FinalImage   : register(t0);
SamplerState PointSampler : register(s0);

float4 TonemappingPS(float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    float3 Color = FinalImage.Sample(PointSampler, TexCoord).rgb;
    Color = ACESFitted(Color);
    Color = ApplyGammaInv(Color);
    return float4(Color, 1.0f);
}