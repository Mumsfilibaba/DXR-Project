#include "Structs.hlsli"
#include "Constants.hlsli"

RWTexture2D<float> Texture : register(u0, space0);

cbuffer Params : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    int2 ScreenSize;
};

#define THREAD_COUNT (16)
#define KERNEL_SIZE  (5)

groupshared float gTextureCache[THREAD_COUNT][THREAD_COUNT];

static const int2 MAX_SIZE = int2(THREAD_COUNT - 1, THREAD_COUNT - 1);

static const float KERNEL[KERNEL_SIZE] =
{
    0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f
};

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
    const int2 TexCoords = int2(Input.DispatchThreadID.xy);
    if (TexCoords.x > ScreenSize.x || TexCoords.y > ScreenSize.y)
    {
        return;
    }
    
    // Cache texture fetches
    const int2 GroupThreadID = int2(Input.GroupThreadID.xy);
    gTextureCache[GroupThreadID.x][GroupThreadID.y] = Texture[TexCoords];
    
    GroupMemoryBarrierWithGroupSync();
    
    // Perform blur
    float Result = 0.0f;
    int   Offset = -((KERNEL_SIZE - 1) / 2);
    
    [unroll]
    for (int Index = 0; Index < KERNEL_SIZE; Index++)
    {
        Offset++;
        
        const float Weight = KERNEL[Index];
        
        // TODO: Handle when we need to sample outside the tile
#ifdef HORIZONTAL_PASS
        const int CurrentTexCoord = max(0, min(MAX_SIZE.x, GroupThreadID.x + Offset));
        Result += gTextureCache[CurrentTexCoord][GroupThreadID.y] * Weight;
#else
        const int CurrentTexCoord = max(0, min(MAX_SIZE.y, GroupThreadID.y + Offset));
        Result += gTextureCache[GroupThreadID.x][CurrentTexCoord] * Weight;
#endif
    }
    
    Texture[TexCoords] = Result;
}