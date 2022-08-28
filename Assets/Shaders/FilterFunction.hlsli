#ifndef FILTER_FUNCTION_HLSLI
#define FILTER_FUNCTION_HLSLI
#include "Constants.hlsli"

#define GAUSSIAN_SIGMA (0.5f)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Filter Functions

float BoxFilter(float Sample)
{
    return (Sample <= 1.0f);
}

float TriangleFilter(float Sample)
{
    return saturate(1.0f - Sample);
}

float GaussianFilter(float Sample)
{
    const float Sigma = GAUSSIAN_SIGMA;
    const float G = 1.0f / sqrt(2.0f * 3.14159f * Sigma * Sigma);
    return (G * exp(-(Sample * Sample) / (2 * Sigma * Sigma)));
}

 float CubicFilter(float Sample, float B, float C)
{
    float y = 0.0f;

    float Sample2 = Sample * Sample;
    float Sample3 = Sample * Sample * Sample;
    if(Sample < 1.0f)
    {   
        y = (12.0f - 9.0f * B - 6.0f * C) * Sample3 + (-18.0f + 12.0f * B + 6.0f * C) * Sample2 + (6.0f - 2.0f * B);
    }
    else if (Sample <= 2.0f)
    {
        y = (-B - 6.0f * C) * Sample3 + (6.0f * B + 30.0f * C) * Sample2 + (-12.0f * B - 48.0f * C) * Sample + (8.0f * B + 24.0f * C);
    }

    return y / 6.0f;
}

float SincFilter(float Sample, float FilterRadius)
{
    float s;
    Sample *= FilterRadius * 2.0f;

    if(Sample < 0.001f)
    {
        s = 1.0f;
    }
    else
    {
        s = sin(Sample * PI) / (Sample * PI);
    }

    return s;
}

float MitchelFilter(float Sample)
{
    return CubicFilter(Sample, 1 / 3.0f, 1 / 3.0f);
}

float BlackmanHarrisFilter(float Sample)
{
    Sample = 1.0f - Sample;

    const float a0 = 0.35875f;
    const float a1 = 0.48829f;
    const float a2 = 0.14128f;
    const float a3 = 0.01168f;
    return saturate(a0 - a1 * cos(PI * Sample) + a2 * cos(2 * PI * Sample) - a3 * cos(3 * PI * Sample));
}

float SmoothstepFilter(float Sample)
{
    return 1.0f - smoothstep(0.0f, 1.0f, Sample);
}

// Modified version of the following code: https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details

float3 SampleTextureCatmullRom(Texture2D<float3> InTexture, SamplerState InLinearSampler, float2 TexCoord, float2 TexSize)
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 SamplePos = TexCoord * TexSize;
    float2 TexPos1 = floor(SamplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = SamplePos - TexPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 Offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 TexPos0  = TexPos1 - 1.0f;
    float2 TexPos3  = TexPos1 + 2.0f;
    float2 TexPos12 = TexPos1 + Offset12;

    TexPos0  /= TexSize;
    TexPos3  /= TexSize;
    TexPos12 /= TexSize;

    float3 Result = 0.0f;
    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos0.x , TexPos0.y), 0.0f) * w0.x  * w0.y;
    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos12.x, TexPos0.y), 0.0f) * w12.x * w0.y;
    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos3.x , TexPos0.y), 0.0f) * w3.x  * w0.y;

    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos0.x , TexPos12.y), 0.0f) * w0.x  * w12.y;
    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos12.x, TexPos12.y), 0.0f) * w12.x * w12.y;
    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos3.x , TexPos12.y), 0.0f) * w3.x  * w12.y;

    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos0.x , TexPos3.y), 0.0f) * w0.x  * w3.y;
    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos12.x, TexPos3.y), 0.0f) * w12.x * w3.y;
    Result += InTexture.SampleLevel(InLinearSampler, float2(TexPos3.x , TexPos3.y), 0.0f) * w3.x  * w3.y;
    return Result;
}

#endif