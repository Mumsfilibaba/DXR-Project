#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

namespace HaltonPrivate
{
    // Modified from this Source: https://pbr-book.org/3ed-2018/Sampling_and_Reconstruction/The_Halton_Sampler
    CONSTEXPR float RadicalInverse2(uint32 Bits)
    {
        Bits = (Bits << 16u) | (Bits >> 16u);
        Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
        Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
        Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
        Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
        return float(double(Bits) * 2.3283064365386963e-10);
    }

    // Modified from this Source: https://pbr-book.org/3ed-2018/Sampling_and_Reconstruction/The_Halton_Sampler
    CONSTEXPR float RadicalInverse3(uint32 Sample)
    {
        constexpr float OneMinusEpsilon = 0x1.fffffep-1;

        const uint32 Base = 3;
        const float InvBase = 1.0f / float(Base);

        uint32 ReversedDigits = 0;
        float InvBaseN = 1.0f;

        while (Sample)
        {
            const uint32 Next  = Sample / Base;
            const uint32 Digit = Sample - Next * Base;
            ReversedDigits = ReversedDigits * Base + Digit;
            InvBaseN *= InvBase;
            Sample = Next;
        }

        return NMath::Min<float>(float(ReversedDigits) * InvBaseN, OneMinusEpsilon);
    }

    inline FVector2 Hammersley2(uint32 Sample, uint32 N)
    {
        return FVector2(float(Sample) / float(N), RadicalInverse2(Sample));
    }

    inline FVector2 Halton23(uint32 Sample)
    {
        return FVector2(RadicalInverse2(Sample), RadicalInverse3(Sample));
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FHaltonState

struct FHaltonState
{
    FHaltonState(const uint32 InMaxNumSamples = 16)
        : MaxNumSamples(InMaxNumSamples)
        , SampleIndex(0)
    { }

    FVector2 NextSample()
    {
        SampleIndex = (SampleIndex + 1) % MaxNumSamples;
        // Avoid the first (0, 0) by adding 1
        FVector2 Sample = HaltonPrivate::Halton23(SampleIndex);
        return (Sample * 2.0f) - 1.0f;
    }

    const uint32 MaxNumSamples;
    uint32       SampleIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTemporalAA

class RENDERER_API FTemporalAA
{
public:
    FTemporalAA()  = default;
    ~FTemporalAA() = default;

    bool Init(FFrameResources& FrameResources);
    void Release();

    void Render(FRHICommandList& CommandList, FFrameResources& FrameResources);

    bool ResizeResources(FFrameResources& FrameResources);

private:
    bool CreateRenderTarget(FFrameResources& FrameResources);

    FRHIComputePipelineStateRef  TemporalAAPSO;
    FRHIComputeShaderRef         TemporalAAShader;

    // Two buffers to ping-pong between
    FRHITexture2DRef             TAAHistoryBuffers[2];
    FRHISamplerStateRef          LinearSampler;

    uint32                       CurrentBufferIndex;
};