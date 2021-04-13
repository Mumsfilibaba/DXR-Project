#include "Helpers.hlsli"
#include "Structs.hlsli"

#define NUM_THREADS 16

Texture2D DepthBuffer : register(t0);

groupshared float MinDepth;
groupshared float MaxDepth;

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoord = Input.DispatchThreadID.xy;
    
    float Depth = DepthBuffer[TexCoord];

}