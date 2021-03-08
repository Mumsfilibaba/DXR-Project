#include "Structs.hlsli"
#include "Constants.hlsli"
#include "helpers.hlsli"

RWTexture2D<float4> Texture : register(u0, space0);

cbuffer Params : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    int2 ScreenSize;
};

#define THREAD_COUNT 16
#define KERNEL_SIZE  5

groupshared float3 gTextureCache[THREAD_COUNT][THREAD_COUNT];

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
    
    //const int2 GroupThreadID = int2(Input.GroupThreadID.xy);
    //gTextureCache[GroupThreadID.x][GroupThreadID.y] = Texture[TexCoords];
    
    float M1 = 0.0f;
    float M2 = 1.0f;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float3 Value    = Texture[TexCoords + int2(x, y)].rgb;
            float LocalLuma = Luma(Value);
            M1 += LocalLuma;
            M2 += LocalLuma * LocalLuma;
        }
    }
    
    M1 = M1 / 9.0f;
    M2 = M2 / 9.0f;
    
    float Variance = M2 - M1 * M1;
    
    GroupMemoryBarrierWithGroupSync();
    
    float3 Result = 0.0f;    
    int Offset = -((KERNEL_SIZE - 1) / 2);
    
    int KernelSize = int(ceil(clamp(1.0f + Variance * 12.0f, 1.0f, 13.0f)));
    Offset = -((KernelSize - 1) / 2);
    
    for (int i = 0; i < KernelSize; i++)
    {
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
        Result += Texture[CurrentTexCoord].rgb;
#else
        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
        Result += Texture[CurrentTexCoord].rgb;
#endif
        Offset++;
    }
    
    Result = Result / KernelSize;
    
//    [unroll]
//    for (int Index = 0; Index < KERNEL_SIZE; Index++)
//    {
//        const float Weight = KERNEL[Index];

//        // TODO: Handle when we need to sample outside the tile
//#ifdef HORIZONTAL_PASS
//        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
//        Result += Texture[CurrentTexCoord] * Weight;
//#else
//        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
//        Result += Texture[CurrentTexCoord] * Weight;
//#endif
        
//        Offset++;
//    }
    
    Texture[TexCoords] = float4(Result, 1.0f);
}