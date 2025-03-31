#ifndef VOGEL_DISK_HLSLI
#define VOGEL_DISK_HLSLI
#include "Constants.hlsli"

float GetNoiseInterleavedGradient(float2 ScreenPos, uint FrameIndex)
{
    const float FrameCount = (float)FrameIndex;
    const float FrameStep  = float(FrameCount % 16) * RPC_16;
    
    ScreenPos.x += FrameStep * 4.7526;
    ScreenPos.y += FrameStep * 3.1914;

    const float3 Magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(Magic.z * frac(dot(ScreenPos, Magic.xy)));
}

float2 VogelDiskSample(uint SampleIndex, uint SampleCount, float Angle)
{
    const float GoldenAngle = 2.399963;

    float R     = sqrt(SampleIndex + 0.5) / sqrt(SampleCount);
    float Theta = SampleIndex * GoldenAngle + Angle;
    
    float Sine;
    float Cosine;
    sincos(Theta, Sine, Cosine);
    
    return float2(Cosine, Sine) * R;
}

#endif