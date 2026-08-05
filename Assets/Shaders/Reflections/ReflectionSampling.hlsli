#ifndef REFLECTION_SAMPLING_HLSLI
#define REFLECTION_SAMPLING_HLSLI

#include "Random.hlsli"
#include "Halton.hlsli"
#include "RayTracingBindless.hlsli"

#define REFLECTION_SAMPLER_WHITE     0
#define REFLECTION_SAMPLER_HALTON    1
#define REFLECTION_SAMPLER_BLUENOISE 2

Texture2D<float2> ReflectionNoise : register(t10); // rg = a jointly distributed pair in [0,1)

float2 SampleReflectionXi(uint2 Pixel, uint2 Dimensions, FRayTracingSceneConstants SceneConstants)
{
    const uint SamplerMode = SceneConstants.ReflectionSampler;
    const uint NoiseSize   = SceneConstants.ReflectionNoiseSize;

    if ((SamplerMode == REFLECTION_SAMPLER_BLUENOISE) && (NoiseSize > 0))
    {
        const float2 Mask    = ReflectionNoise.Load(int3(int2(Pixel % NoiseSize), 0));
        const float2 Advance = float2(0.7548776662466927f, 0.5698402909980532f) * float(SceneConstants.FrameIndex);
        return frac(Mask + Advance);
    }
    else if (SamplerMode == REFLECTION_SAMPLER_HALTON)
    {
        uint         OffsetSeed = InitRandom(Pixel, Dimensions.x, 0);
        const float2 Offset     = NextRandom2(OffsetSeed);
        return frac(Halton23(SceneConstants.FrameIndex + 1) + Offset);
    }

    uint Seed = InitRandom(Pixel, Dimensions.x, SceneConstants.FrameIndex);
    return NextRandom2(Seed);
}

#endif
