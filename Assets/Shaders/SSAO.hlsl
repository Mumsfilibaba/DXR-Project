#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Random.hlsli"

Texture2D<float3> GBufferNormals : register(t0, space0);
Texture2D<float>  GBufferDepth   : register(t1, space0);

SamplerState GBufferSampler : register(s0, space0);

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

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(FComputeShaderInput Input)
{
    const int   FinalKernelSize = min(max(KernelSize, 4), MAX_SAMPLES);
    const float FinalRadius = max(Radius, 0.01f);
    const float FinalBias   = max(Bias, 0.0f);

    // Texture coordinate
    const float2 TexSize   = ScreenSize;
    const uint2  Pixel     = min(Input.DispatchThreadID.xy, uint2(TexSize));   
    const float2 TexCoords = (float2(Pixel) + 0.5f) / TexSize;

    // Unpack Normal
    float3 ViewNormal = GBufferNormals.SampleLevel(GBufferSampler, TexCoords, 0).rgb;
    if (length(ViewNormal) == 0)
    {
        Output[Pixel] = 1.0f;
        return;
    }
    
    ViewNormal = UnpackNormal(ViewNormal);

    const float4x4 Projection    = CameraBuffer.Projection;
    const float4x4 ProjectionInv = CameraBuffer.ProjectionInv;

    // Get the depth and calculate view-space position
    const float  Depth        = GBufferDepth.SampleLevel(GBufferSampler, TexCoords, 0);
    const float3 ViewPosition = PositionFromDepth(Depth, TexCoords, ProjectionInv); 
    
    // Random Seed
    uint RandomSeed = InitRandom(Pixel, uint2(TexSize).x, 0);

	const float3 NoiseVec   = normalize(NextRandom3(RandomSeed) * 2.0f - 1.0f);
    const float3 Tangent    = normalize(NoiseVec - ViewNormal * dot(NoiseVec, ViewNormal));
    const float3 Bitangent  = cross(ViewNormal, Tangent);
    float3x3 TBN = float3x3(Tangent, Bitangent, ViewNormal);

    float Occlusion = 0.0f;
    for (int Index = 0; Index < FinalKernelSize; ++Index)
    {
		const float2 HammerslySample = Hammersley2(Index, FinalKernelSize);
		const float3 Sample          = HemispherePointUniform(HammerslySample.x, HammerslySample.y);
        
        float  RayLength = FinalRadius * NextRandom(RandomSeed);
        float3 SamplePos = normalize(mul(Sample, TBN));
        SamplePos = ViewPosition + (SamplePos * RayLength);

        float4 SampleProjected = mul(Projection, float4(SamplePos, 1.0f));
        SampleProjected.xyz = (SampleProjected.xyz / SampleProjected.w);
        SampleProjected.xy  = (SampleProjected.xy * float2(0.5f, -0.5f)) + 0.5f;
        
        [branch]
		if (all(SampleProjected.xy >= 0.0f) && all(SampleProjected.xy <= 1.0f))
		{
            float SampleDepth = GBufferDepth.SampleLevel(GBufferSampler, SampleProjected.xy, 0);
            SampleDepth = Depth_ProjToView(SampleDepth, ProjectionInv);

            const float RangeCheck = 1.0f - saturate(abs(SampleProjected.w - SampleDepth) * FinalRadius);
            Occlusion += float(SampleDepth < (SampleProjected.w - FinalBias)) * RangeCheck;
        }
    }
    
    Occlusion = 1.0f - (Occlusion / float(FinalKernelSize));
    Output[Pixel] = Occlusion;
}