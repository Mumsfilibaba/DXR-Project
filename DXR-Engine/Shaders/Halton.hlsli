#ifndef HALTON_HLSLI
#define HALTON_HLSLI

// Modifed from this Source: https://pbr-book.org/3ed-2018/Sampling_and_Reconstruction/The_Halton_Sampler
float RadicalInverse2(uint Bits)
{
    Bits = (Bits << 16u) | (Bits >> 16u);
    Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
    Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
    Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
    Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
    return float(Bits) * 2.3283064365386963e-10;
}

// Modifed from this Source: https://pbr-book.org/3ed-2018/Sampling_and_Reconstruction/The_Halton_Sampler
float RadicalInverse3(uint a)
{
    const float OneMinusEpsilon = 0x1.fffffep-1;
    
    const uint  Base = 3;
    const float InvBase = 1.0f / float(Base);
    
    uint  ReversedDigits = 0;
    float InvBaseN = 1.0f;
    
    while (a)
    {
        uint Next  = a / Base;
        uint Digit = a - Next * Base;
        ReversedDigits = ReversedDigits * Base + Digit;
        InvBaseN *= InvBase;
        a = Next;
    }
    
    return min(ReversedDigits * InvBaseN, OneMinusEpsilon);
}

float2 Hammersley2(uint i, uint n)
{
    return float2(float(i) / float(n), RadicalInverse2(i));
}

float Halton23(uint i)
{
    return float2(RadicalInverse2(i), RadicalInverse3(i));
}

#endif