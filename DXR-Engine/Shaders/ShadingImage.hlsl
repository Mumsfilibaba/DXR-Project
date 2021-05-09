#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Helpers.hlsli"

// NOTE: Needs to change based on VRS image tile size
#define NUM_THREADS       (16)
#define NUM_TOTAL_THREADS (NUM_THREADS * NUM_THREADS)

#ifndef ENABLE_HALF_RES 
    #define ENABLE_HALF_RES 1
#endif

#ifndef VRS_IMAGE_ROUGHNESS 
    #define VRS_IMAGE_ROUGHNESS 1
#endif

Texture2D<float4> GBufferMaterialTex : register(t0);
Texture2D<float4> GBufferNormalTex   : register(t0);
Texture2D<float4> GBufferDepthTex    : register(t1);

RWTexture2D<uint> Output : register(u0);

ConstantBuffer<Camera> CameraBuffer : register(b0);

groupshared float Averages[NUM_TOTAL_THREADS];

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
    
    const int2 Resolution = int2(CameraBuffer.Width, CameraBuffer.Height);
    const int2 Group = int2(Input.GroupID.xy);
    const uint GroupThreadIndex = Input.GroupThreadID.y * NUM_THREADS + Input.GroupThreadID.x;

#if ENABLE_HALF_RES
    const int TileSize = 2;
#else
    const int TileSize = 1;
#endif
    
    const int2 Pixel = int2(Input.DispatchThreadID.xy) * TileSize;
    
#if VRS_IMAGE_ROUGHNESS
    float LocalAvg = 0.0f;
    
    [unroll]
    for (int y = 0; y < TileSize; y++)
    {
        [unroll]
        for (int x = 0; x < TileSize; x++)
        {
            LocalAvg += GBufferMaterialTex[Pixel + int2(x, y)].r;
        }
    }
    
    LocalAvg = LocalAvg / float(TileSize * TileSize);
    Averages[GroupThreadIndex] = LocalAvg;
    
    GroupMemoryBarrierWithGroupSync();
    
	[unroll(NUM_TOTAL_THREADS)]
    for (uint i = NUM_TOTAL_THREADS / 2; i > 0; i >>= 1)
    {
        if (GroupThreadIndex < i)
        {
            Averages[GroupThreadIndex] += Averages[GroupThreadIndex + i];
        }

        GroupMemoryBarrierWithGroupSync();
    }
    
    if (GroupThreadIndex == 0)
    {
        float Avg = Averages[0] / (float)NUM_TOTAL_THREADS;
    
        uint ShadingRate = SHADING_RATE_1x1;
        if (Avg > 0.75f)
        {
            ShadingRate = SHADING_RATE_2x2;
        }
        else if (Avg > 0.5f)
        {
            ShadingRate = SHADING_RATE_2x1;
        }
    
        Output[Group] = ShadingRate;
    }
    
#else
    float LocalAvg = 0.0f;
    
    [unroll]
    for (int y = 0; y < TileSize; y++)
    {
        [unroll]
        for (int x = 0; x < TileSize; x++)
        {
            int2 CurrentTexCoord = Pixel + int2(x, y);
            float2 UV = float2(CurrentTexCoord) / float2(Resolution);
            
            float  Depth = GBufferDepthTex[CurrentTexCoord];
            float3 Position = PositionFromDepth(Depth, UV, CameraBuffer.ViewProjectionInverse);

            float3 N = GBufferNormalTex[CurrentTexCoord].rgb;
            N = UnpackNormal(N);
            
            float3 V = normalize(CameraBuffer.Position - Position);
            
            float VdotN = 1.0 - saturate(dot(V, N));
            LocalAvg += VdotN;
        }
    }
    
    LocalAvg = LocalAvg / float(TileSize * TileSize);
    Averages[GroupThreadIndex] = LocalAvg;
    
    GroupMemoryBarrierWithGroupSync();
    
    [unroll(NUM_TOTAL_THREADS)]
    for (uint i = NUM_TOTAL_THREADS / 2; i > 0; i >>= 1)
    {
        if (GroupThreadIndex < i)
        {
            Averages[GroupThreadIndex] += Averages[GroupThreadIndex + i];
        }

        GroupMemoryBarrierWithGroupSync();
    }
    
    if (GroupThreadIndex == 0)
    {
        float Avg = Averages[0] / (float)NUM_TOTAL_THREADS;
        
        uint ShadingRate = SHADING_RATE_2x2;
        if (Avg > 0.5f)
        {
            ShadingRate = SHADING_RATE_1x1;
        }
        else if (Avg > 0.25f)
        {
            ShadingRate = SHADING_RATE_2x1;
        }
    
        Output[Group] = ShadingRate;
    }
#endif
}