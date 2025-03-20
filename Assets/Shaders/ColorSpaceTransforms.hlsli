#ifndef IMAGE_BASED_LIGHTING_HLSLI
#define IMAGE_BASED_LIGHTING_HLSLI
#include "Constants.hlsli"

// Converts from sRGB gamma-encoded color space to linear color space
// sRGB uses piecewise gamma curve - linear segment for dark colors, power curve for brighter
float3 SRGBToLinear(float3 Color)
{
    // Linear segment for dark colors (below ~0.04045)
    float3 LinearLow = Color / 12.92;
    
    // Power curve segment for bright colors (gamma correction)
    float3 LinearHigh = pow((Color + 0.055) / 1.055, STANDARD_GAMMA);
    
    // Create mask (0 or 1) to select between curve segments
    // 0.0404482362771082 is the exact sRGB threshold (≈0.04045)
    float3 IsHigh = step(0.0404482362771082, Color);
    
    // Blend between the two curve results based on brightness
    return lerp(LinearLow, LinearHigh, IsHigh);
}

// Converts from linear color space back to sRGB gamma-encoded space
float3 LinearToSRGB(float3 Color)
{
    // Linear segment for dark colors (below ~0.0031308)
    float3 SRGBLow = Color * 12.92;
    
    // Power curve segment for bright colors (inverse gamma correction)
    float3 SRGBHigh = 1.055 * pow(Color, 1.0 / STANDARD_GAMMA) - 0.055;
    
    // Threshold for inverse transform (≈0.0031308)
    float3 IsHigh = step(0.00313066844250063, Color);
    
    return lerp(SRGBLow, SRGBHigh, IsHigh);
}

// Converts linear color values to HDR10 color space with PQ curve
float3 LinearToHDR10(float3 Color, float WhitePoint)
{
    // Color space conversion from Rec.709 (sRGB) to Rec.2020 (HDR)
    // Necessary because HDR10 uses wider Rec.2020 color gamut
    {
        // BT.709 to BT.2020 matrix conversion
        static const float3x3 From709To2020 =
        {   
            { 0.6274040f, 0.3292820f, 0.0433136f }, // Red channel coefficients
            { 0.0690970f, 0.9195400f, 0.0113612f }, // Green channel coefficients
            { 0.0163916f, 0.0880132f, 0.8955950f }  // Blue channel coefficients
        };
        
        Color = mul(From709To2020, Color);
    }

    // Normalize color values for PQ curve application
    // ST2084 (PQ curve) is defined up to 10,000 nits
    // WhitePoint = display's maximum brightness capability (in nits)
    const float ST2084Max = 10000.0f;
    Color *= WhitePoint / ST2084Max;  // Scale to PQ's normalized range

    // Apply SMPTE ST 2084 (Perceptual Quantizer) curve
    // PQ curve encodes high dynamic range into 0-1 range while preserving perceptual details
    {
        // Constants from SMPTE ST 2084 specification
        static const float M1 = 2610.0 / 4096.0 / 4;   // 0.1593017578125
        static const float M2 = 2523.0 / 4096.0 * 128; // 78.84375
        static const float C1 = 3424.0 / 4096.0;       // 0.8359375
        static const float C2 = 2413.0 / 4096.0 * 32;  // 18.8515625
        static const float C3 = 2392.0 / 4096.0 * 32;  // 18.6875
        
        float3 CP = pow(abs(Color), M1);  // Absolute value for safety (input should be positive)
        Color = pow((C1 + C2 * CP) / (1 + C3 * CP), M2);
    }

    return Color;
}

#endif