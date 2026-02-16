#include "../Structs.hlsli"
#include "../Helpers.hlsli"
#include "../FilterFunction.hlsli"

#define NUM_THREADS (16)

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float> Output : register(u0);
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float> HistoryOut : register(u1);

Texture2D<float>  DepthBuffer    : register(t0);
Texture2D<float2> VelocityBuffer : register(t1);
Texture2D<float>  ShadowMaskRaw  : register(t2);
Texture2D<float>  HistoryBuffer  : register(t3);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SamplerState LinearSampler : register(s0);

uint2 ClampToViewport(uint2 TexCoord, int2 Offset)
{
    const int2 ViewportSize = int2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    int2 CurrentPosition = int2(TexCoord) + Offset;
    CurrentPosition = clamp(CurrentPosition, int2(0, 0), ViewportSize.xy - 1);
    return uint2(CurrentPosition);
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 TexCoord = DispatchThreadID.xy;

    float SampleTotal  = 0.0;
    float SampleWeight = 0.0;
    float MinSample = FLT32_MAX;
    float MaxSample = -FLT32_MAX;
    float Moment0 = 0.0;
    float Moment1 = 0.0;

    float ClosestDepth = FLT32_MAX;
    int2  ClosestDepthPixelPosition = int2(0, 0);

    for (int OffsetX = -1; OffsetX <= 1; ++OffsetX)
    {
        for (int OffsetY = -1; OffsetY <= 1; ++OffsetY)
        {
            const uint2 CurrentPosition = ClampToViewport(TexCoord, int2(OffsetX, OffsetY));
            const float SubSampleDistance = length(float2(OffsetX, OffsetY));
            const float SubSampleWeight   = BlackmanHarrisFilter(SubSampleDistance);

            const float SubSample = ShadowMaskRaw[CurrentPosition];
            SampleTotal  += SubSample * SubSampleWeight;
            SampleWeight += SubSampleWeight;

            MinSample = min(MinSample, SubSample);
            MaxSample = max(MaxSample, SubSample);

            Moment0 += SubSample;
            Moment1 += SubSample * SubSample;

            const float CurrentDepth = DepthBuffer[CurrentPosition];
            if (CurrentDepth < ClosestDepth)
            {
                ClosestDepth = CurrentDepth;
                ClosestDepthPixelPosition = int2(CurrentPosition);
            }
        }
    }

    const float CurrentSample = SampleTotal / max(SampleWeight, 1e-6);

    const float2 MotionVector = VelocityBuffer[ClosestDepthPixelPosition].xy * float2(0.5, -0.5);
    const float2 TexCoordUV   = (float2(TexCoord) + 0.5) / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);

    const float2 HistoryTexCoord = TexCoordUV - MotionVector;
    if (any(HistoryTexCoord != saturate(HistoryTexCoord)))
    {
        Output[TexCoord] = CurrentSample;
        HistoryOut[TexCoord] = CurrentSample;
        return;
    }

    // Variance clamp
    const float OneDividedBySampleCount = 1.0 / 9.0;
    const float Gamma = 1.0;
    const float MomentU = Moment0 * OneDividedBySampleCount;
    const float Sigma = sqrt(abs((Moment1 * OneDividedBySampleCount) - (MomentU * MomentU)));
    const float MinColor = MomentU - (Gamma * Sigma);
    const float MaxColor = MomentU + (Gamma * Sigma);

    float HistorySample = HistoryBuffer.SampleLevel(LinearSampler, HistoryTexCoord, 0).r;
    HistorySample = clamp(HistorySample, MinSample, MaxSample);
    const float3 Clipped = ClipAABB(float3(MinColor, MinColor, MinColor), float3(MaxColor, MaxColor, MaxColor), float3(HistorySample, HistorySample, HistorySample));
    HistorySample = Clipped.x;

    const float SourceWeight  = 0.1;
    const float HistoryWeight = 0.9;
    const float NewSample = ((CurrentSample * SourceWeight) + (HistorySample * HistoryWeight)) / max(SourceWeight + HistoryWeight, 1e-6);

    Output[TexCoord] = NewSample;
    HistoryOut[TexCoord] = NewSample;
}
