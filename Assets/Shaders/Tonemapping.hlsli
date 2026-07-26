#ifndef TONEMAPPING_HLSLI
#define TONEMAPPING_HLSLI

#include "CoreDefines.hlsli"
#include "Helpers.hlsli"
#include "Constants.hlsli"

#define TONEMAPPING_TYPE_UNKNOWN 0
#define TONEMAPPING_TYPE_ACES 1
#define TONEMAPPING_TYPE_REINHARD 2
#define TONEMAPPING_TYPE_UNCHARTED_2 3

// Reinhard Tonemapping
float3 ReinhardMapping(float3 Color, float Intensity = 1.0)
{
    return Color / (Intensity + Color);
}

// ACES Tonemapping
float3 RTTAndODTFit(float3 V)
{
    float3 A = V * (V + 0.0245786) - 0.000090537;
    float3 B = V * (0.983729 * V + 0.4329510) + 0.238081;
    return A / B;
}

float3 ACESFitted(float3 Color)
{
    // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
    const float3x3 InputMatrix =
    {
        { 0.59719, 0.35458, 0.04823 },
        { 0.07600, 0.90834, 0.01566 },
        { 0.02840, 0.13383, 0.83777 },
    };

    Color = mul(InputMatrix, Color);

    // RRTAndODTFit
    Color = RTTAndODTFit(Color);

    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    const float3x3 OutputMatrix =
    {
        {  1.60475, -0.53108, -0.07367 },
        { -0.10208,  1.10813, -0.00605 },
        { -0.00327, -0.07276,  1.07602 },
    };

    return saturate(mul(OutputMatrix, Color));
}

// Tonemapping used in Uncharted 2
float3 Uncharted2(float3 Color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((Color * (A * Color + C * B) + D * E) / (Color * (A * Color + B) + D * F)) - E / F;
}

#endif