#include "Constants.hlsli"
#include "Tonemapping.hlsli"
#include "ColorSpaceTransforms.hlsli"

Texture2D FinalImage : register(t0);
SamplerState PointSampler : register(s0);

SHADER_CONSTANT_BLOCK_BEGIN
    int   TonemappingType;
    int   OutputSRGB;
    float ReinhardIntensity;
    float ExposureEV100;
SHADER_CONSTANT_BLOCK_END

float4 TonemappingPS(float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    float3 Color = FinalImage.Sample(PointSampler, TexCoord).rgb;
    Color *= exp2(-Constants.ExposureEV100);

    switch (Constants.TonemappingType)
    {
        case TONEMAPPING_TYPE_REINHARD:
        {
            Color = ReinhardMapping(Color, Constants.ReinhardIntensity);
            break;
        }

        case TONEMAPPING_TYPE_UNCHARTED_2:
        {
            Color = Uncharted2(Color);
            break;
        }

        case TONEMAPPING_TYPE_UNKNOWN:
        case TONEMAPPING_TYPE_ACES:
        default:
        {
            Color = ACESFitted(Color);
            break;
        }
    }

    if (Constants.OutputSRGB != 0)
    {
        Color = LinearToSRGB(Color);
    }

    return float4(Color, 1.0);
}
