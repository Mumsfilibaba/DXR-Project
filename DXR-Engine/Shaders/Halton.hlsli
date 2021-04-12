#ifndef HALTON_HLSLI
#define HALTON_HLSLI

float RadicalInverseVdC(uint Bits)
{
    Bits = (Bits << 16u) | (Bits >> 16u);
    Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
    Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
    Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
    Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
    
    // Bits * (1/2^32)
    return float(Bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint I, uint N)
{
    return float2(float(I) / float(N), RadicalInverseVdC(I));
}

#endif