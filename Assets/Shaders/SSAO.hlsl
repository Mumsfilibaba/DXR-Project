#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Random.hlsli"

#define THREAD_COUNT 16
#define MAX_SAMPLES 128
#define ENABLE_FRAME_INDEX 0

Texture2D<float3> GBufferNormals : register(t0);
Texture2D<float>  GBufferDepth   : register(t1);

SamplerState GBufferSampler : register(s0);

RWTexture2D<float> Output : register(u0);

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    float2 ScreenSize;
    float2 NoiseSize;

    // 16-32
    int2  GBufferSize;
    float Radius;
    float Bias;
    
    // 32-40
    uint KernelSize;
    uint FrameIndex;
SHADER_CONSTANT_BLOCK_END

ConstantBuffer<FCamera> CameraBuffer : register(b0);

groupshared float3 HaltonSamples[MAX_SAMPLES];

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(FComputeShaderInput Input)
{
    // Start with generating the samples for this group
    const uint KernelSize = min(max(Constants.KernelSize, 1), MAX_SAMPLES);
    if (Input.GroupIndex < KernelSize)
    {
        const float2 HammerslySample    = Hammersley2(Input.GroupIndex, KernelSize);
        HaltonSamples[Input.GroupIndex] = HemispherePointUniform(HammerslySample.x, HammerslySample.y);
    }

    GroupMemoryBarrierWithGroupSync();

    // Texture coordinate
    const float2 TexSize   = Constants.ScreenSize;
    const uint2  Pixel     = Input.DispatchThreadID.xy;   
    const float2 TexCoords = ((float2)Pixel) / TexSize;

    // Early out on pixels not rendered to
    float Depth = GBufferDepth.SampleLevel(GBufferSampler, TexCoords, 0);
    if (Depth == 1.0)
    {
        Output[Pixel] = 1.0;
        return;
    }
    
    // Unpack Normal
    float3 Normal = GBufferNormals.SampleLevel(GBufferSampler, TexCoords, 0).rgb;
    Normal = UnpackNormal(Normal);
    Normal = mul(float4(Normal, 0.0f), CameraBuffer.View).xyz;

    const float Radius = max(Constants.Radius, 0.01);
    const float Bias   = max(Constants.Bias, 0.0);

    const float4x4 Projection    = CameraBuffer.Projection;
    const float4x4 ProjectionInv = CameraBuffer.ProjectionInv;

    // Get the depth and calculate view-space position
    const float3 ViewPosition = PositionFromDepth(Depth, TexCoords, ProjectionInv);
    
    // Random Seed
#if ENABLE_FRAME_INDEX
    const uint FrameIndex = Constants.FrameIndex;
#else
    const uint FrameIndex = 0;
#endif

    uint RandomSeed = InitRandom(Pixel, uint2(TexSize).x, FrameIndex);

    // Construct a tangent-space matrix
	const float3 Noise     = NextRandom3(RandomSeed) * 2.0 - 1.0;
    const float3 Tangent   = normalize(Noise - Normal * dot(Noise, Normal));
    const float3 Bitangent = cross(Normal, Tangent);
    
    float3x3 TangentSpace = float3x3(Tangent, Bitangent, Normal);

    // Trace screen-space AO
    float Occlusion = 0.0f;
    for (uint Index = 0; Index < KernelSize; ++Index)
    {
        const float3 Sample = HaltonSamples[Index];
        
        // Randomize the ray-length
        float RayLength = Radius * lerp(0.2, 1.0, NextRandom(RandomSeed));

        // Construct the final sample
        float3 SamplePos = mul(Sample, TangentSpace);
        SamplePos = ViewPosition + (SamplePos * RayLength);

        // Project sample
        float4 SampleProjected = mul(float4(SamplePos, 1.0), Projection);
        SampleProjected.xyz = (SampleProjected.xyz / SampleProjected.w);
        SampleProjected.xy  = (SampleProjected.xy * float2(0.5, -0.5)) + 0.5;
        
        [branch]
		if (all(SampleProjected.xy >= 0.0) && all(SampleProjected.xy <= 1.0))
		{
            float SampleDepth = GBufferDepth.SampleLevel(GBufferSampler, SampleProjected.xy, 0);
            SampleDepth = Depth_ProjToView(SampleDepth, ProjectionInv);

            const float RangeCheck = 1.0 - saturate(abs(SampleProjected.w - SampleDepth) * Radius);
            Occlusion += float(SampleDepth < (SampleProjected.w - Bias)) * RangeCheck;
        }
    }
    
    Occlusion = 1.0 - (Occlusion / float(KernelSize));
    Output[Pixel] = Occlusion;
}