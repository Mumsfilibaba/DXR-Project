#include "Structs.hlsli"
#include "Constants.hlsli"

RWTexture2D<float> Texture : register(u0, space0);

cbuffer Params : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    int2 ScreenSize;
};

#define NUM_THREADS (16)
#define KERNEL_SIZE (5)

groupshared float GTextureCache[NUM_THREADS][NUM_THREADS];

static const int2 MAX_SIZE = int2(NUM_THREADS, NUM_THREADS);

static const float KERNEL[KERNEL_SIZE] =
{
    0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f
};

static const int OFFSETS[KERNEL_SIZE] =
{
    -2, -1, 0, 1, 2
};

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(FComputeShaderInput Input)
{
    const int2 Pixel = min(Input.DispatchThreadID.xy, int2(ScreenSize));   

    // Cache texture fetches
    const int2 GroupThreadID = int2(Input.GroupThreadID.xy);
    GTextureCache[GroupThreadID.x][GroupThreadID.y] = Texture[Pixel];
    
    GroupMemoryBarrierWithGroupSync();
    
    // Perform blur
    float Result = 0.0f;
    [unroll]
    for (int Index = 0; Index < KERNEL_SIZE; ++Index)
    {
        const int   Offset = OFFSETS[Index];
        const float Weight = KERNEL[Index];
        
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(GroupThreadID.x + Offset, GroupThreadID.y);
#else
        const int2 CurrentTexCoord = int2(GroupThreadID.x, GroupThreadID.y + Offset);
#endif
        // Going outside of the cache? 
        if (any(CurrentTexCoord >= MAX_SIZE) || any(CurrentTexCoord < int2(0, 0)))
        {
#ifdef HORIZONTAL_PASS
        const int2 CurrentPixel = int2(min(max(Pixel.x + Offset, 0), ScreenSize.x), Pixel.y);
#else
        const int2 CurrentPixel = int2(Pixel.x, min(max(Pixel.y + Offset, 0), ScreenSize.y));
#endif
            Result += Texture[Pixel] * Weight;
        }
        else
        {
            Result += GTextureCache[CurrentTexCoord.x][CurrentTexCoord.y] * Weight;
        }
    }
    
    Texture[Pixel] = Result;
}