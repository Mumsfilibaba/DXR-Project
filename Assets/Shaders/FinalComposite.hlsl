#include "CoreDefines.hlsli"
#include "ColorSpaceTransforms.hlsli"

Texture2D TonemappedImage : register(t0);
Texture2D<float> SelectionRing : register(t1);

SamplerState PointSampler  : register(s0);
SamplerState LinearSampler : register(s1);

SHADER_CONSTANT_BLOCK_BEGIN
    int    EnableSelectionOutline;
    float  OutlineAlpha;
    float2 Padding0;

    float3 OutlineColor;
    float  Padding1;
SHADER_CONSTANT_BLOCK_END

float4 Main(float2 TexCoord : TEXCOORD0) : SV_TARGET0
{
    float3 Color = TonemappedImage.Sample(PointSampler, TexCoord).rgb;

    if (Constants.EnableSelectionOutline != 0)
    {
        const float Ring = SelectionRing.Sample(LinearSampler, TexCoord).r;
        Color = lerp(Color, Constants.OutlineColor, Ring * Constants.OutlineAlpha);
    }

    Color = LinearToSRGB(Color);
    return float4(Color, 1.0);
}
