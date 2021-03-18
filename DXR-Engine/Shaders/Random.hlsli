#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

float Random(float3 Seed, int i)
{
    float4 Seed4 = float4(Seed, i);
    float Dot = dot(Seed4, float4(12.9898f, 78.233f, 45.164f, 94.673f));
    return frac(sin(Dot) * 43758.5453f);
}

// Source: https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
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

#endif