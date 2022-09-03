#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Random.hlsli"

Texture2D<float3> GBufferNormals : register(t0, space0);
Texture2D<float>  GBufferDepth   : register(t1, space0);
Texture2D<float3> Noise          : register(t2, space0);

StructuredBuffer<float4> Samples : register(t3, space0);

SamplerState GBufferSampler : register(s0, space0);
SamplerState NoiseSampler   : register(s1, space0);

RWTexture2D<float> Output : register(u0, space0);

cbuffer Params : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float2 ScreenSize;
    float2 NoiseSize;
    int2   GBufferSize;

    float  Radius;
    float  Bias;
    int    KernelSize;
};

ConstantBuffer<FCamera> CameraBuffer : register(b0, space0);

#define THREAD_COUNT (16)
#define MAX_SAMPLES  (64)

groupshared float3 SamplesCache[MAX_SAMPLES];

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(FComputeShaderInput Input)
{
    const int   FinalKernelSize = min(max(KernelSize, 4), MAX_SAMPLES);
    const float FinalRadius = max(Radius, 0.01f);
    const float FinalBias   = max(Bias, 0.0f);

    // Cache all the samples
    const uint GroupThreadIndex = Input.GroupIndex;
    if (GroupThreadIndex < FinalKernelSize)
    {
        SamplesCache[GroupThreadIndex] = Samples[GroupThreadIndex].xyz;
    }

    // Output coordinate
    const float2 TexSize         = ScreenSize;
    const uint2  OutputTexCoords = min(Input.DispatchThreadID.xy, uint2(TexSize));   

    // GBuffer coordinate
    const float2 TexCoords    = (float2(OutputTexCoords) + 0.5f) / TexSize;
    const uint2  GBufferPixel = uint2(TexCoords * float2(GBufferSize)); 

    const float4x4 Projection        = CameraBuffer.Projection;
    const float4x4 ProjectionInverse = CameraBuffer.ProjectionInverse;

    // Get the depth and calculate view-space position
    const float Depth   = GBufferDepth[GBufferPixel];
    float3 ViewPosition = PositionFromDepth(Depth, TexCoords, ProjectionInverse); 
    
    // Unpack Normal
    float3 ViewNormal = GBufferNormals[GBufferPixel].rgb;
    ViewNormal = UnpackNormal(ViewNormal);
    
    const float2 NoiseScale = TexSize / NoiseSize;
    const float3 NoiseVec   = normalize(Noise.SampleLevel(NoiseSampler, TexCoords * NoiseScale, 0));
    
    const float3 Tangent    = normalize(NoiseVec - ViewNormal * dot(NoiseVec, ViewNormal));
    const float3 Bitangent  = cross(ViewNormal, Tangent);
    float3x3 TBN = float3x3(Tangent, Bitangent, ViewNormal);
    
    GroupMemoryBarrierWithGroupSync();

    float Occlusion = 0.0f;
    for (int i = 0; i < FinalKernelSize; ++i)
    {
        const float3 Sample = SamplesCache[i];
        
        float3 SamplePos = mul(Sample, TBN);
        SamplePos = ViewPosition + (SamplePos * FinalRadius);
            
        float4 Offset = mul(Projection, float4(SamplePos, 1.0f));
        Offset.xyz = Offset.xyz / Offset.w;
        Offset.xy  = (Offset.xy * 0.5f) + 0.5f;
        Offset.y   = 1.0f - Offset.y;
        
        float SampleDepth = GBufferDepth.SampleLevel(GBufferSampler, Offset.xy, 0);
        SampleDepth = Depth_ProjToView(SampleDepth, ProjectionInverse);
        
        const float RangeCheck = smoothstep(0.0f, 1.0f, FinalRadius / abs(ViewPosition.z - SampleDepth));
        Occlusion += (SampleDepth >= (SamplePos.z - Bias) ? 0.0f : 1.0f) * RangeCheck;
    }
    
    Occlusion = 1.0f - (Occlusion / float(FinalKernelSize));
    Output[OutputTexCoords] = Occlusion;
}