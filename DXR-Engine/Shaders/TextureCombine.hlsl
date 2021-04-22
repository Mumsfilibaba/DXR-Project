#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Helpers.hlsli"

#define NUM_THREADS 16

Texture2D<float3> Source : register(t0);
Texture2D<float>  Alpha  : register(t1);

RWTexture2D<float4> Output : register(u0);

SamplerState Sampler : register(s0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void AddAlpha_Main(ComputeShaderInput Input)
{
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
 
    uint2 Pixel = Input.DispatchThreadID.xy;
    if (Pixel.x > Width || Pixel.y > Height)
    {
        return;
    }
    
    float2 TexCoord = float2(Pixel) / float2(Width, Height);
    
    float4 NewColor = Float4(0.0f);
    NewColor.rgb = Source.SampleLevel(Sampler, TexCoord, 0.0f);
    NewColor.a   = Alpha.SampleLevel(Sampler, TexCoord, 0.0f);
    
    Output[Pixel] = NewColor;
}

Texture2D<float> Source0 : register(t0);
Texture2D<float> Source1 : register(t1);
Texture2D<float> Source2 : register(t2);
Texture2D<float> Source3 : register(t3);

cbuffer CombineChannelsSettings : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    int EnableSource0;
    int EnableSource1;
    int EnableSource2;
    int EnableSource3;
};

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void CombineChannels_Main(ComputeShaderInput Input)
{
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
 
    uint2 Pixel = Input.DispatchThreadID.xy;
    if (Pixel.x > Width || Pixel.y > Height)
    {
        return;
    }
    
    float2 TexCoord = float2(Pixel) / float2(Width, Height);
    
    float4 NewColor = Float4(0.0f);
    if (EnableSource0)
    {
        NewColor.r = Source0.SampleLevel(Sampler, TexCoord, 0.0f);
    }
    
    if (EnableSource1)
    {
        NewColor.g = Source1.SampleLevel(Sampler, TexCoord, 0.0f);
    }

    if (EnableSource2)
    {
        NewColor.b = Source2.SampleLevel(Sampler, TexCoord, 0.0f);
    }
    
    if (EnableSource3)
    {
        NewColor.a = Source3.SampleLevel(Sampler, TexCoord, 0.0f);
    }
    
    Output[Pixel] = NewColor;
}