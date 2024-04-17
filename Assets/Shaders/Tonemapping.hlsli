#ifndef TONEMAPPING_HLSLI
#define TONEMAPPING_HLSLI
#include "CoreDefines.hlsli"
#include "Helpers.hlsli"
#include "Constants.hlsli"

// Gamma
float3 ApplyGamma(float3 Color)
{
    return pow(Color, Float3(GAMMA));
}

float3 ApplyGammaInv(float3 InputColor)
{
    return pow(InputColor, Float3(1.0f / GAMMA));
}

// Reinhard Tonemapping
float3 SimpleReinhardMapping(float3 Color, float Intensity)
{
    return Color / (Float3(Intensity) + Color);
}

// ACES Tonemapping
float3 RTTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 Color)
{
    const float3x3 InputMatrix =
    {
        { 0.59719f, 0.35458f, 0.04823f },
        { 0.07600f, 0.90834f, 0.01566f },
        { 0.02840f, 0.13383f, 0.83777f },
    };

    const float3x3 OutputMatrix =
    {
        { 1.60475f, -0.53108f, -0.07367f },
        { -0.10208f, 1.10813f, -0.00605f },
        { -0.00327f, -0.07276f, 1.07602f },
    };

    Color = mul(InputMatrix, Color);
    Color = RTTAndODTFit(Color);
    return saturate(mul(OutputMatrix, Color));
}

#endif