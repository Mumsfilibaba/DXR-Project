#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

// Based on: https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
uint InitRandom(uint2 Pixel, uint Width, uint FrameIndex, uint BackOff = 16)
{
    uint v0 = Pixel.x + Pixel.y * Width;
    uint v1 = FrameIndex;
    uint s0 = 0;

    [unroll]
    for (uint n = 0; n < BackOff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    
    return v0;
}

// Xorshift*32
// Based on George Marsaglia's work: http://www.jstatsoft.org/v08/i14/paper
uint XORShift(uint Value)
{
    Value ^= Value << 13;
    Value ^= Value >> 17;
    Value ^= Value << 5;
    return Value;
}

float NextRandom(inout uint Seed)
{
    Seed = XORShift(Seed);
    return float(Seed) * (1.0f / 4294967296.0f);
}

int NextRandomInt(inout uint Seed)
{
    Seed = XORShift(Seed);
    return Seed;
}

float NextRandomNeg(inout uint Seed)
{
    return NextRandom(Seed) * 2.0f - 1.0f;
}

float CranleyPatterssonRotation(float v, inout uint Seed)
{
    return frac(v + NextRandom(Seed));
}

float2 CranleyPatterssonRotation(float2 v, inout uint Seed)
{
    v.x = frac(v.x + NextRandom(Seed));
    v.y = frac(v.y + NextRandom(Seed));
    return v;
}

float3 CranleyPatterssonRotation(float3 v, inout uint Seed)
{
    v.x = frac(v.x + NextRandom(Seed));
    v.y = frac(v.y + NextRandom(Seed));
    v.z = frac(v.z + NextRandom(Seed));
    return v;
}

#endif