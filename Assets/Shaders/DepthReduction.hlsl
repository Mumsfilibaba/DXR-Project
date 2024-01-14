#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

#define NUM_THREADS       (16)
#define NUM_THREADS_TOTAL (NUM_THREADS * NUM_THREADS)
#define REVERSED_DEPTH    (0)

// Handles first reduction
Texture2D<float>  DepthBuffer : register(t0);
Texture2D<float2> InputMinMax : register(t0);

RWTexture2D<float2> OutputMinMax : register(u0);

SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 CamProjection;
    float    NearPlane;
    float    FarPlane;
SHADER_CONSTANT_BLOCK_END

groupshared float GroupMinZ[NUM_THREADS_TOTAL];
groupshared float GroupMaxZ[NUM_THREADS_TOTAL];

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void ReductionMainInital(FComputeShaderInput Input)
{  
    // Retrieve thread indicies
    uint2 TextureSize;
    DepthBuffer.GetDimensions(TextureSize.x, TextureSize.y);
    
    uint2 TexCoords = Input.GroupID.xy * NUM_THREADS + Input.GroupThreadID.xy;
    TexCoords = min(TexCoords, TextureSize - 1);

    const uint GroupThreadIndex = Input.GroupIndex;
   
    // Start reduction
    float4x4 Projection = transpose(Constants.CamProjection);
    
    float MinDepth = 1.0f;
    float MaxDepth = 0.0f;
    
    // Linearize depth 1/z
    float DepthSample = DepthBuffer[TexCoords];
    if (DepthSample < 1.0f)
    {
        DepthSample = Projection._43 / (DepthSample - Projection._33);
        DepthSample = saturate((DepthSample - Constants.NearPlane) / (Constants.FarPlane - Constants.NearPlane));
        MinDepth = min(MinDepth, DepthSample);
        MaxDepth = max(MaxDepth, DepthSample);
    }
    
    GroupMinZ[GroupThreadIndex] = MinDepth;
    GroupMaxZ[GroupThreadIndex] = MaxDepth;
    
    GroupMemoryBarrierWithGroupSync();
    
    // Parallel reduction
	[unroll]
	for(uint i = NUM_THREADS_TOTAL / 2; i > 0; i >>= 1)
    {
        if (NUM_THREADS_TOTAL < i)
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
[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void ReductionMain(FComputeShaderInput Input)
{
        // Retrieve thread indicies
    uint2 TextureSize;
    InputMinMax.GetDimensions(TextureSize.x, TextureSize.y);
    
    uint2 TexCoords = Input.GroupID.xy * NUM_THREADS + Input.GroupThreadID.xy;
    TexCoords = min(TexCoords, TextureSize - 1);

    const uint GroupThreadIndex = Input.GroupIndex;
    
    // Start reduction
    float2 MinMaxSample = InputMinMax[TexCoords];
    if (MinMaxSample.x == 0.0f)
    {
        MinMaxSample.x = 1.0f;
    }

    GroupMinZ[GroupThreadIndex] = MinMaxSample.x;
    GroupMaxZ[GroupThreadIndex] = MinMaxSample.y;
    
    GroupMemoryBarrierWithGroupSync();
    
    // Parallel Reduction
    for (uint i = NUM_THREADS_TOTAL / 2; i > 0; i >>= 1)
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