#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

#define THREAD_COUNT       (16)
#define TOTAL_THREAD_COUNT (THREAD_COUNT * THREAD_COUNT)

// Handles first reduction
Texture2D<float>  DepthBuffer : register(t0);
Texture2D<float2> InputMinMax : register(t0);

RWTexture2D<float2> OutputMinMax : register(u0);

cbuffer Constants : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 CamProjection;
    float    NearPlane;
    float    FarPlane;
};

groupshared float GroupMinZ[TOTAL_THREAD_COUNT];
groupshared float GroupMaxZ[TOTAL_THREAD_COUNT];

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void ReductionMainInital(FComputeShaderInput Input)
{
    uint Width;
    uint Height;
    DepthBuffer.GetDimensions(Width, Height);
    
    uint2 TexCoords = min(Input.DispatchThreadID.xy, uint2(Width - 1, Height - 1));
    
    uint GroupThreadIndex = Input.GroupThreadID.y * THREAD_COUNT + Input.GroupThreadID.x;
   
    float4x4 Projection = transpose(CamProjection);
    
    float MinDepth = 1.0f;
    float MaxDepth = 0.0f;
    
    // Linearize depth 1/z
    float DepthSample = DepthBuffer[TexCoords];
    if (DepthSample < 1.0f)
    {
        DepthSample = Projection._43 / (DepthSample - Projection._33);
        DepthSample = saturate((DepthSample - NearPlane) / (FarPlane - NearPlane));
        MinDepth = min(MinDepth, DepthSample);
        MaxDepth = max(MaxDepth, DepthSample);
    }
    
    GroupMinZ[GroupThreadIndex] = MinDepth;
    GroupMaxZ[GroupThreadIndex] = MaxDepth;
    
    GroupMemoryBarrierWithGroupSync();
    
    // Parallel reduction
    for (uint i = TOTAL_THREAD_COUNT / 2; i > 0; i >>= 1)
    {
        if (GroupThreadIndex < i)
        {
            GroupMinZ[GroupThreadIndex] = min(GroupMinZ[GroupThreadIndex], GroupMinZ[GroupThreadIndex + i]);
            GroupMaxZ[GroupThreadIndex] = max(GroupMaxZ[GroupThreadIndex], GroupMaxZ[GroupThreadIndex + i]);
        }
        
        GroupMemoryBarrierWithGroupSync();
    }
    
    if (GroupThreadIndex == 0)
    {
        OutputMinMax[Input.GroupID.xy] = float2(GroupMinZ[0], GroupMaxZ[0]);
    }
}

// Handles the rest of the Reductions
[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void ReductionMain(FComputeShaderInput Input)
{
    uint Width;
    uint Height;
    InputMinMax.GetDimensions(Width, Height);
    
    uint2 TexCoords = min(Input.DispatchThreadID.xy, uint2(Width - 1, Height - 1));
    
    uint GroupThreadIndex = Input.GroupThreadID.y * THREAD_COUNT + Input.GroupThreadID.x;
    
    float2 MinMaxSample = InputMinMax[TexCoords];
    if (MinMaxSample.x == 0.0f)
    {
        MinMaxSample.x = 1.0f;
    }
    
    GroupMinZ[GroupThreadIndex] = MinMaxSample.x;
    GroupMaxZ[GroupThreadIndex] = MinMaxSample.y;
    
    GroupMemoryBarrierWithGroupSync();
    
    // Parallel Reduction
    for (uint i = TOTAL_THREAD_COUNT / 2; i > 0; i >>= 1)
    {
        if (GroupThreadIndex < i)
        {
            GroupMinZ[GroupThreadIndex] = min(GroupMinZ[GroupThreadIndex], GroupMinZ[GroupThreadIndex + i]);
            GroupMaxZ[GroupThreadIndex] = max(GroupMaxZ[GroupThreadIndex], GroupMaxZ[GroupThreadIndex + i]);
        }
        
        GroupMemoryBarrierWithGroupSync();
    }
    
    if (GroupThreadIndex == 0)
    {
        OutputMinMax[Input.GroupID.xy] = float2(GroupMinZ[0], GroupMaxZ[0]);
    }
}