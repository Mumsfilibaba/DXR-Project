#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Helpers.hlsli"

// NOTE: Needs to change based on VRS image tile size
#define NUM_THREADS 4

#ifndef TRACE_HALF_RES 
    #define TRACE_HALF_RES 0
#endif

#ifndef VRS_IMAGE_ROUGHNESS 
    #define VRS_IMAGE_ROUGHNESS 0
#endif

Texture2D<float4> GBufferMaterialTex : register(t0);
Texture2D<float4> GBufferNormalTex   : register(t0);
Texture2D<float4> GBufferDepthTex    : register(t1);

RWTexture2D<uint> Output : register(u0);

ConstantBuffer<Camera> CameraBuffer : register(b0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
    
    const uint2 Resolution = uint2(CameraBuffer.Width, CameraBuffer.Height);
    
#if TRACE_HALF_RES
    const int TileSize = 16 * 2;
#else
    const int TileSize = 16;
#endif
    
    int2 OutputTexCoord = int2(Input.DispatchThreadID.xy);
    int2 TexCoord = OutputTexCoord * TileSize;
    
#if VRS_IMAGE_ROUGHNESS
    float Avg = 0.0f;
    for (int y = 0; y < TileSize; y++)
    {
        for (int x = 0; x < TileSize; x++)
        {
            Avg += GBufferMaterialTex[TexCoord + int2(x, y)].r;
        }
    }
    
    Avg = Avg / float(TileSize * TileSize);
    
    uint Result = SHADING_RATE_1x1;
    if (Avg > 0.7f)
    {
        Result = SHADING_RATE_4x4;
    }
    else if (Avg > 0.35f)
    {
        Result = SHADING_RATE_2x4;
    }
    else if (Avg > 0.15f)
    {
        Result = SHADING_RATE_2x2;
    }
    else if (Avg > 0.075f)
    {
        Result = SHADING_RATE_2x1;
    }
#else
    float Avg = 0.0f;
    for (int y = 0; y < TileSize; y++)
    {
        for (int x = 0; x < TileSize; x++)
        {
            int2 CurrentTexCoord = TexCoord + int2(x, y);
            float2 UV = float2(CurrentTexCoord) / float2(Resolution);
            
            float  Depth = GBufferDepthTex[CurrentTexCoord];
            float3 Position = PositionFromDepth(Depth, UV, CameraBuffer.ViewProjectionInverse);

            float3 N = GBufferNormalTex[CurrentTexCoord].rgb;
            N = UnpackNormal(N);
            
            float3 V = normalize(CameraBuffer.Position - Position);
            
            float VdotN = 1.0 - saturate(dot(V, N));
            Avg += VdotN;
        }
    }
    
    Avg = Avg / float(TileSize * TileSize);
    
    uint Result = SHADING_RATE_2x2;
    if (Avg > 0.7f)
    {
        Result = SHADING_RATE_1x1;
    }
    else if (Avg > 0.4f)
    {
        Result = SHADING_RATE_2x1;
    }
#endif
    
    Output[OutputTexCoord] = Result;
}