#include "PBRCommon.hlsli"

Texture2D FinalImage		: register(t0, space0);
SamplerState PointSampler	: register(s0, space0);

float4 Main(float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    float3 Color = FinalImage.Sample(PointSampler, TexCoord).rgb;
    return float4(Color, 1.0f);
}