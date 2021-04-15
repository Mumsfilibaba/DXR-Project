#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

#define THREAD_COUNT 16

// Handles first reduction
Texture2D<float> DepthBuffer : register(t0);

Texture2D<float2> InputMinMax : register(t0);

RWTexture2D<float2> OutputMinMax : register(u0);

cbuffer Constants : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 CamProjection;
    float NearPlane;
    float FarPlane;
};

groupshared uint GroupMinZ;
groupshared uint GroupMaxZ;

void InitGroupShared(uint GroupThreadIndex)
{
    if (GroupThreadIndex == 0)
    {
        GroupMinZ = 0x7f7fffff;
        GroupMaxZ = 0;
    }
}

void WriteResults(uint GroupThreadIndex, uint2 TexCoords)
{
    if (GroupThreadIndex == 0)
    {
        float MinZ = asfloat(GroupMinZ);
        float MaxZ = asfloat(GroupMaxZ);
        OutputMinMax[TexCoords] = float2(MinZ, MaxZ);
    }
}

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void ReductionMainInital(ComputeShaderInput Input)
{
    uint2 TexCoords = Input.DispatchThreadID.xy;
    
    uint Width;
    uint Height;
    DepthBuffer.GetDimensions(Width, Height);
    
    if (TexCoords.x > Width || TexCoords.y > Height)
    {
        return;
    }
    
    uint GroupThreadIndex = Input.GroupThreadID.y * THREAD_COUNT + Input.GroupThreadID.x;
    InitGroupShared(GroupThreadIndex);
    
    float4x4 Projection = transpose(CamProjection);
    
    float DepthSample = DepthBuffer[TexCoords];
    DepthSample = Projection._43 / (DepthSample - Projection._33);
    DepthSample = saturate((DepthSample - NearPlane) / (FarPlane - NearPlane));
    
    uint Depth = asuint(DepthSample);
    InterlockedMin(GroupMinZ, Depth);
    InterlockedMax(GroupMaxZ, Depth);

    GroupMemoryBarrierWithGroupSync();
    
    WriteResults(GroupThreadIndex, Input.GroupID.xy);
}

// Handles the rest of the reductions
[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void ReductionMain(ComputeShaderInput Input)
{
    uint2 TexCoords = Input.DispatchThreadID.xy;
    
    uint Width;
    uint Height;
    InputMinMax.GetDimensions(Width, Height);
    
    if (TexCoords.x > Width || TexCoords.y > Height)
    {
        return;
    }
    
    uint GroupThreadIndex = Input.GroupThreadID.y * THREAD_COUNT + Input.GroupThreadID.x;
    InitGroupShared(GroupThreadIndex);
    
    float2 MinMaxSample = InputMinMax[TexCoords];
    if (MinMaxSample.x == 0.0f)
    {
        MinMaxSample.x = 1.0f;
    }
    
    uint2 MinMax = asuint(MinMaxSample);
    InterlockedMin(GroupMinZ, MinMax.x);
    InterlockedMax(GroupMaxZ, MinMax.y);

    GroupMemoryBarrierWithGroupSync();
    
    WriteResults(GroupThreadIndex, Input.GroupID.xy);
}