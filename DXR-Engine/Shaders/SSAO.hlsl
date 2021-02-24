#include "PBRHelpers.hlsli"
#include "Structs.hlsli"

Texture2D<float3> GBufferNormals : register(t0, space0);
Texture2D<float>  GBufferDepth   : register(t1, space0);
Texture2D<float3> Noise          : register(t2, space0);

StructuredBuffer<float3> Samples : register(t3, space0);

SamplerState GBufferSampler : register(s0, space0);
SamplerState NoiseSampler   : register(s1, space0);

RWTexture2D<float> Output : register(u0, space0);

cbuffer Params : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float2 ScreenSize;
    float2 NoiseSize;
    float  Radius;
    float  Bias;
    int    KernelSize;
};

ConstantBuffer<Camera> CameraBuffer : register(b0, space0);

#define THREAD_COUNT 16
#define MAX_SAMPLES  64

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
    const uint2 OutputTexCoords = Input.DispatchThreadID.xy;
    if (Input.DispatchThreadID.x > uint(ScreenSize.x) || Input.DispatchThreadID.y > uint(ScreenSize.y))
    {
        return;
    }
    
    const float2 TexCoords = (float2(OutputTexCoords) + 0.5f) / ScreenSize;
    const float  Depth  = GBufferDepth.SampleLevel(GBufferSampler, TexCoords, 0);
    float3 ViewPosition = PositionFromDepth(Depth, TexCoords, CameraBuffer.ProjectionInverse);
    
    float3 ViewNormal = GBufferNormals.SampleLevel(GBufferSampler, TexCoords, 0).rgb;
    ViewNormal = UnpackNormal(ViewNormal);
    
    const float2 NoiseScale = ScreenSize / NoiseSize;
    const float3 NoiseVec   = normalize(Noise.SampleLevel(NoiseSampler, TexCoords * NoiseScale, 0));
    const float3 Tangent    = normalize(NoiseVec - ViewNormal * dot(NoiseVec, ViewNormal));
    const float3 Bitangent  = cross(ViewNormal, Tangent);
    float3x3 TBN = float3x3(Tangent, Bitangent, ViewNormal);
    
    const float FinalRadius     = max(Radius, 0.05f);
    const float FinalBias       = max(Bias, 0.0f);
    const int   FinalKernelSize = max(KernelSize, 4);
    
    float Occlusion = 0.0f;
    for (int i = 0; i < FinalKernelSize; i++)
    {
        const int Index = int(float(MAX_SAMPLES) * Random(floor(ViewPosition.xyz * 1000.0f), i));
        
        const float3 Sample = Samples[Index];
        float3 SamplePos = mul(Sample, TBN);
        SamplePos = ViewPosition + SamplePos * FinalRadius;
            
        float4 Offset = mul(CameraBuffer.Projection, float4(SamplePos, 1.0f));
        Offset.xy = Offset.xy / Offset.w;
        Offset.xy = (Offset.xy * 0.5f) + 0.5f;
        Offset.y  = 1.0f - Offset.y;
        
        float SampleDepth = GBufferDepth.SampleLevel(GBufferSampler, Offset.xy, 0);
        float3 DepthPos   = PositionFromDepth(SampleDepth, Offset.xy, CameraBuffer.ProjectionInverse);
        SampleDepth = DepthPos.z;
        
        const float RangeCheck = smoothstep(0.0f, 1.0f, FinalRadius / abs(SampleDepth - ViewPosition.z));
        Occlusion += (SampleDepth >= (SamplePos.z - FinalBias) ? 0.0f : 1.0f) * RangeCheck;
    }
    
    Occlusion = 1.0f - (Occlusion / float(FinalKernelSize));
    Occlusion = Occlusion * Occlusion;
    Output[OutputTexCoords] = Occlusion;
}