#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

#define THREAD_COUNT 16

Texture2D<float> DepthBuffer : register(t0);

RWTexture2D<float2> DepthMinMax : register(u0);

groupshared uint GroupMinZ;
groupshared uint GroupMaxZ;

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoords = Input.DispatchThreadID.xy;
    
    uint GroupThreadIndex = Input.GroupThreadID.y * THREAD_COUNT + Input.GroupThreadID.x;
    if (GroupThreadIndex == 0)
    {
        GroupMinZ = 0x7f7fffff;
        GroupMaxZ = 0;
    }
    
    uint Depth = asuint(DepthBuffer[TexCoords]);
    InterlockedMin(GroupMinZ, Depth);
    InterlockedMax(GroupMaxZ, Depth);

    GroupMemoryBarrierWithGroupSync();
    
    if (GroupThreadIndex == 0)
    {
        float MinZ = asfloat(GroupMinZ);
        float MaxZ = asfloat(GroupMaxZ);
        
        uint2 OutTexCoord = Input.GroupID.xy;
        DepthMinMax[OutTexCoord] = float2(MinZ, MaxZ);
    }
}