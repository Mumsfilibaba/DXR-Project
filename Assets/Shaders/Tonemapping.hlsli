#ifndef TONEMAPPING_HLSLI
#define TONEMAPPING_HLSLI
#include "CoreDefines.hlsli"
#include "Helpers.hlsli"
#include "Constants.hlsli"

// Gamma
float3 ApplyGamma(float3 Color)
{
    return pow(Color, GAMMA);
}

float3 ApplyGammaInv(float3 InputColor)
{
    return pow(InputColor, 1.0 / GAMMA);
}

// Reinhard Tonemapping
float3 SimpleReinhardMapping(float3 Color, float Intensity)
{
    return Color / (Intensity + Color);
}

// ACES Tonemapping
float3 RTTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

float3 ACESFitted(float3 Color)
{
    const float3x3 InputMatrix =
    {
        { 0.59719, 0.35458, 0.04823 },
        { 0.07600, 0.90834, 0.01566 },
        { 0.02840, 0.13383, 0.83777 },
    };

    const float3x3 OutputMatrix =
    {
        {  1.60475, -0.53108, -0.07367 },
        { -0.10208,  1.10813, -0.00605 },
        { -0.00327, -0.07276,  1.07602 },
    };

    Color = mul(InputMatrix, Color);
    Color = RTTAndODTFit(Color);
    return saturate(mul(OutputMatrix, Color));
}

#endif