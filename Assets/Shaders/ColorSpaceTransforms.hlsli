#ifndef COLOR_SPACE_TRANSFORMS_HLSLI
#define COLOR_SPACE_TRANSFORMS_HLSLI

#include "Constants.hlsli"

float3 SRGBToLinear(float3 Color)
{
    float3 LinearLow  = Color / 12.92;
    float3 LinearHigh = pow((Color + 0.055) / 1.055, STANDARD_GAMMA);
    float3 IsHigh     = step(0.0404482362771082, Color);
    
    return lerp(LinearLow, LinearHigh, IsHigh);
}

float3 LinearToSRGB(float3 Color)
{
    float3 SRGBLow  = Color * 12.92;
    float3 SRGBHigh = 1.055 * pow(Color, 1.0 / STANDARD_GAMMA) - 0.055;
    float3 IsHigh   = step(0.00313066844250063, Color);
    
    return lerp(SRGBLow, SRGBHigh, IsHigh);
}

float3 LinearToHDR10(float3 Color, float WhitePoint)
{
    {
        // BT.709 to BT.2020 matrix conversion
        static const float3x3 From709To2020 =
        {   
            { 0.6274040f, 0.3292820f, 0.0433136f },
            { 0.0690970f, 0.9195400f, 0.0113612f },
            { 0.0163916f, 0.0880132f, 0.8955950f } 
        };
        
        Color = mul(From709To2020, Color);
    }

    const float ST2084Max = 10000.0f;
    Color *= WhitePoint / ST2084Max;

    {
        static const float M1 = 2610.0 / 4096.0 / 4;  
        static const float M2 = 2523.0 / 4096.0 * 128;
        static const float C1 = 3424.0 / 4096.0;      
        static const float C2 = 2413.0 / 4096.0 * 32; 
        static const float C3 = 2392.0 / 4096.0 * 32; 
        
        float3 CP = pow(abs(Color), M1);
        Color = pow((C1 + C2 * CP) / (1 + C3 * CP), M2);
    }

    return Color;
}

#endif