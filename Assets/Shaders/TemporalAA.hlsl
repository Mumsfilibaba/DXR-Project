#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Helpers.hlsli"
#include "FilterFunction.hlsli"

#define NUM_THREADS (16)

RWTexture2D<float4> FinalTarget : register(u0, space0);
RWTexture2D<float4> Output      : register(u1, space0);

Texture2D<float>  DepthBuffer    : register(t0, space0);
Texture2D<float2> VelocityBuffer : register(t1, space0);
Texture2D<float3> HistoryBuffer  : register(t2, space0);

ConstantBuffer<FCamera> CameraBuffer : register(b0, space0);

SamplerState LinearSampler : register(s0, space0);

uint2 ClampToViewport(uint2 TexCoord, int2 Offset)
{
    const int2 ViewportSize = int2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight); 
    
    int2 CurrentPosition = int2(TexCoord) + Offset;
    CurrentPosition = clamp(CurrentPosition, int2(0, 0), ViewportSize.xy - 1);
    return uint2(CurrentPosition);
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(FComputeShaderInput Input)
{
    const uint2 TexCoord = Input.DispatchThreadID.xy;

    // Sample neightbourhood and find the maxvalues
    float3 SampleTotal  = Float3(0.0f);
    float  SampleWeight = 0.0f;

    // Min and Max samples from this frame1
    float3 MinSample = Float3( FLT32_MAX);
    float3 MaxSample = Float3(-FLT32_MAX);
    
    // For variance calculation
    float3 Moment0 = Float3(0.0f);
    float3 Moment1 = Float3(0.0f);

    // Helpers
    float ClosestDepth = FLT32_MAX;
    int2  ClosestDepthPixelPosition = int2(0, 0);

    for (int OffsetX = -1; OffsetX <= 1; ++OffsetX)
    {
        for (int OffsetY = -1; OffsetY <= 1; ++OffsetY)
        {
            // Retrieve the position but clamp to the edges of the texture
            const uint2 CurrentPosition = ClampToViewport(TexCoord, int2(OffsetX, OffsetY));

            // To retrive the sample we sample the neightbourhood and do some filtering to "un-jitter the image"
            const float SubSampleDistance = length(float2(OffsetX, OffsetY));
            const float SubSampleWeight   = BlackmanHarrisFilter(SubSampleDistance);

            // Sample and ensure its a valid sample
            float3 SubSample = FinalTarget[CurrentPosition].rgb;
            SubSample = max(Float3(0.0f), SubSample);

            SampleTotal  += SubSample * SubSampleWeight;
            SampleWeight += SubSampleWeight;

            MinSample = min(MinSample, SubSample);
            MaxSample = max(MaxSample, SubSample);

            Moment0 += SubSample;
            Moment1 += SubSample * SubSample;

            const float CurrentDepth = DepthBuffer[CurrentPosition];
            if (CurrentDepth < ClosestDepth)
            {
                ClosestDepth              = CurrentDepth;
                ClosestDepthPixelPosition = CurrentPosition;
            }
        }   
    }

    const float3 CurrentSample = SampleTotal / SampleWeight;
    
    // Calculate and test the history-coordinate
    const float2 MotionVector = VelocityBuffer[ClosestDepthPixelPosition].xy * float2(0.5f, -0.5f);
    const float2 TexCoordUV   = (float2(TexCoord) + Float2(0.5f)) / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);

    const float2 HistoryTexCoord = TexCoordUV - MotionVector;
    if(any(HistoryTexCoord != saturate(HistoryTexCoord)))
    {
        Output[TexCoord]      = float4(CurrentSample, 1.0f);
        FinalTarget[TexCoord] = float4(CurrentSample, 1.0f);
        return;
    }

    // Variance Calculation
    const float OneDividedBySampleCount = 1.0f / 9.0f;
    const float Gamma = 1.0f;

    float3 MomentU  = Moment0 * OneDividedBySampleCount;
    float3 Sigma    = sqrt(abs((Moment1 * OneDividedBySampleCount) - (MomentU * MomentU)));
    float3 MinColor = MomentU - (Gamma * Sigma);
    float3 MaxColor = MomentU + (Gamma * Sigma);
 
    // TODO: Try and use a proper catmull rom sampling method using bilinear filtering
    // Clip and Clamp History sample to better fit the new frame
    float3 HistorySample = SampleTextureCatmullRom(HistoryBuffer, LinearSampler, HistoryTexCoord, float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight));
    HistorySample = clamp(HistorySample, MinSample, MaxSample);
    HistorySample = ClipAABB(MinColor, MaxColor, HistorySample);

    const float3 CompressedSource  = CurrentSample * rcp(max(max(CurrentSample.r, CurrentSample.g), CurrentSample.b) + 1.0f);
    const float3 CompressedHistory = HistorySample * rcp(max(max(HistorySample.r, HistorySample.g), HistorySample.b) + 1.0f);
    
    const float LuminanceSource  = Luminance(CompressedSource);
    const float LuminanceHistory = Luminance(CompressedHistory); 
    
    // Calculate weights
    float SourceWeight  = 0.05f;
    float HistoryWeight = 1.0f - SourceWeight;
    SourceWeight  *= 1.0f / (1.0f + LuminanceSource);
    HistoryWeight *= 1.0f / (1.0f + LuminanceHistory);

    const float3 NewSample = ((CurrentSample * SourceWeight) + (HistorySample * HistoryWeight)) / max(SourceWeight + HistoryWeight, 0.00001);
    FinalTarget[TexCoord] = float4(NewSample, 1.0f);
    Output[TexCoord]      = float4(NewSample, 1.0f);
}