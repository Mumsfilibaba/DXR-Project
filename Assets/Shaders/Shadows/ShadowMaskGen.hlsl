#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "Halton.hlsli"
#include "CascadeStructs.hlsli"

#define CASCADE_SHADOW_SPLITS_REGISTER             t1
#define CASCADE_SHADOW_CASCADES_REGISTER           t4
#define CASCADE_SHADOW_SAMPLER_POINT_CMP_REGISTER  s0
#define CASCADE_SHADOW_SAMPLER_LINEAR_CMP_REGISTER s1
#define CASCADE_SHADOW_SAMPLER_POINT_REGISTER      s2
#include "CascadeShadowSampling.hlsli"

#if !defined(NUM_THREADS)
    #define NUM_THREADS 16
#endif

#if !defined(ENABLE_DEBUG)
    #define ENABLE_DEBUG 0
#endif

struct FDirectionalShadowSettings
{
    // 0-16
    float FilterSize;
    float MaxFilterSize;
    uint  ShadowMapSize;
    uint  FrameIndex;

    // 16-32
    uint  NumSamples;
    uint  Padding0;
    uint  Padding1;
    uint  Padding2;
};

#if SHADER_BACKEND == SHADER_BACKEND_METAL
    ConstantBuffer<FCamera>           CameraBuffer : register(b2);
    ConstantBuffer<FDirectionalLight> LightBuffer  : register(b3);
#else
    ConstantBuffer<FCamera>           CameraBuffer : register(b0);
    ConstantBuffer<FDirectionalLight> LightBuffer  : register(b1);
#endif

ConstantBuffer<FDirectionalShadowSettings> SettingsBuffer : register(b2);

StructuredBuffer<FCascadeMatrices> ShadowMatricesBuffer : register(t0);
Texture2D<float>                   DepthBuffer          : register(t2);
Texture2D<float3>                  NormalBuffer         : register(t3);

// Output
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float> Output : register(u0);
#if ENABLE_DEBUG
    TEXTURE_FORMAT_UNKNOWN RWTexture2D<uint> CascadeIndexTex : register(u1);
#endif

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 Pixel = DispatchThreadID.xy;
   
    // Discard pixels not rendered to the GBuffer
    const float Depth = DepthBuffer.Load(int3(Pixel, 0)); 
    if (Depth == 1.0)
    {
        Output[Pixel] = 1.0;
        return;
    }

    const float2 PixelCenter   = float2(Pixel) + 0.5;
    const float2 TexCoord      = PixelCenter / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    const float3 PositionWS    = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    const float3 GBufferNormal = NormalBuffer.Load(int3(Pixel, 0));
    const float3 Normal        = UnpackNormal(GBufferNormal);

    // Initialize random-seed
    uint RandomSeed = InitRandom(Pixel, CameraBuffer.ViewportWidth, 0);
    
    // Cascade-index is written to for debug purposes
    uint CascadeIndex = 0;

    FCascadeShadowContext Context;
    Context.Light                  = LightBuffer;
    Context.Settings.FilterSize    = SettingsBuffer.FilterSize;
    Context.Settings.MaxFilterSize = SettingsBuffer.MaxFilterSize;
    Context.Settings.ShadowMapSize = SettingsBuffer.ShadowMapSize;
    Context.Settings.NumSamples    = SettingsBuffer.NumSamples;

    // Calculate the Shadow
    const float ViewPosZ     = Depth_ProjToView(Depth, CameraBuffer.ProjectionInv);
    const float ShadowAmount = ComputeCascadeShadow(Context, PositionWS, Normal, ViewPosZ, CascadeIndex, RandomSeed);
    Output[Pixel] = ShadowAmount;

    // Output debug-information needed when visualizing the cascades
#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}
