#include "Structs.hlsli"
#include "Constants.hlsli"

// NOTE: Needs to change based on VRS image tile size
#define NUM_THREADS    4
#define TRACE_HALF_RES 0

Texture2D<float4> GBufferMaterialTex : register(t0);

RWTexture2D<uint> Output : register(u0);

groupshared float Avg;

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
    
    const int TileSize = 
#if TRACE_HALF_RES
    16 * 2;
#else
    16;
#endif
    
    int2 OutputTexCoord = int2(Input.DispatchThreadID.xy);
    int2 TexCoord       = OutputTexCoord * TileSize;
    
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
    if (Avg > 0.66f)
    {
        Result = SHADING_RATE_4x4;
    }
    else if (Avg > 0.33f)
    {
        Result = SHADING_RATE_2x2;
    }
    
    Output[OutputTexCoord] = Result;
}