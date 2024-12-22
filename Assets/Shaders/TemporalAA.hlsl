#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Helpers.hlsli"
#include "FilterFunction.hlsli"

#define NUM_THREADS (16)
#define FLT_EPS (0.00000001)
#define HDR_CORRECTION (1)

RWTexture2D<float4> FinalTarget : register(u0);
RWTexture2D<float4> Output      : register(u1);

Texture2D<float>  DepthBuffer    : register(t0);
Texture2D<float2> VelocityBuffer : register(t1);
Texture2D<float3> HistoryBuffer  : register(t2);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SamplerState LinearSampler : register(s0);

uint2 ClampToViewport(uint2 TexCoord, int2 Offset)
{
    const int2 ViewportSize = int2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight); 
    
    int2 CurrentPosition = int2(TexCoord) + Offset;
    CurrentPosition = clamp(CurrentPosition, int2(0, 0), ViewportSize.xy - 1);
    return uint2(CurrentPosition);
}

float3 Tonemap(float3 x)
{
    // Reinhard tonemap
    return x / (x + 1.0);
}

float3 InvTonemap(float3 x)
{
    return x / max(1.0 - x, FLT_EPS);
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(FComputeShaderInput Input)
{
    const uint2 TexCoord = Input.DispatchThreadID.xy;

    // Sample neightbourhood and find the maxvalues
    float3 SampleTotal  = 0.0;
    float  SampleWeight = 0.0;

    // Min and Max samples from this frame1
    float3 MinSample = FLT32_MAX;
    float3 MaxSample = -FLT32_MAX;
    
    // For variance calculation
    float3 Moment0 = 0.0;
    float3 Moment1 = 0.0;

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
        #if HDR_CORRECTION
            // Take HDR into account
            float3 SubSample = Tonemap(FinalTarget[CurrentPosition].rgb);
        #else
            float3 SubSample = FinalTarget[CurrentPosition].rgb;
        #endif   
            SubSample = max(0.0, SubSample);

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

    float3 CurrentSample = SampleTotal / SampleWeight;

    // Calculate and test the history-coordinate
    const float2 MotionVector = VelocityBuffer[ClosestDepthPixelPosition].xy * float2(0.5, -0.5);
    const float2 TexCoordUV   = (float2(TexCoord) + 0.5) / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);

    const float2 HistoryTexCoord = TexCoordUV - MotionVector;
    if(any(HistoryTexCoord != saturate(HistoryTexCoord)))
    {
    #if HDR_CORRECTION
        CurrentSample = InvTonemap(CurrentSample);
    #endif

        Output[TexCoord]      = float4(CurrentSample, 1.0);
        FinalTarget[TexCoord] = float4(CurrentSample, 1.0);
        return;
    }

    // Variance Calculation
    const float OneDividedBySampleCount = 1.0 / 9.0;
    const float Gamma = 1.0;

    float3 MomentU  = Moment0 * OneDividedBySampleCount;
    float3 Sigma    = sqrt(abs((Moment1 * OneDividedBySampleCount) - (MomentU * MomentU)));
    float3 MinColor = MomentU - (Gamma * Sigma);
    float3 MaxColor = MomentU + (Gamma * Sigma);
 
    // TODO: Try and use a proper catmull rom sampling method using bilinear filtering
    // Clip and Clamp History sample to better fit the new frame
    float3 HistorySample = SampleTextureCatmullRom(HistoryBuffer, LinearSampler, HistoryTexCoord, float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight));
#if HDR_CORRECTION
    HistorySample = Tonemap(HistorySample);
#endif    
    HistorySample = clamp(HistorySample, MinSample, MaxSample);
    HistorySample = ClipAABB(MinColor, MaxColor, HistorySample);

    const float3 CompressedSource  = CurrentSample * rcp(max(max(CurrentSample.r, CurrentSample.g), CurrentSample.b) + 1.0);
    const float3 CompressedHistory = HistorySample * rcp(max(max(HistorySample.r, HistorySample.g), HistorySample.b) + 1.0);
    
    const float LuminanceSource  = Luminance(CompressedSource);
    const float LuminanceHistory = Luminance(CompressedHistory); 
    
    // Calculate weights
#if HDR_CORRECTION
    float SourceWeight  = 0.1;
#else
    float SourceWeight  = 0.05;
#endif

    float HistoryWeight = 1.0 - SourceWeight;
    SourceWeight  *= 1.0 / (1.0 + LuminanceSource);
    HistoryWeight *= 1.0 / (1.0 + LuminanceHistory);

    float3 NewSample = ((CurrentSample * SourceWeight) + (HistorySample * HistoryWeight)) / max(SourceWeight + HistoryWeight, 0.00001);
#if HDR_CORRECTION
    NewSample = InvTonemap(NewSample);
#endif

    FinalTarget[TexCoord] = float4(NewSample, 1.0);
    Output[TexCoord]      = float4(NewSample, 1.0);
}