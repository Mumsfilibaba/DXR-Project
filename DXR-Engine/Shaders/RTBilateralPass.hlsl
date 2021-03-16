#include "Structs.hlsli"
#include "Constants.hlsli"
#include "helpers.hlsli"

RWTexture2D<float4> Texture : register(u0, space0);

cbuffer Params : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    int2 ScreenSize;
};

#define THREAD_COUNT 16

float3 Kernel_3(int2 TexCoords)
{
    static const float KERNEL[3] =
    {
        0.27901f,
        0.44198f,
        0.27901f,
    };
    
    int Offset = -1;
    float3 Result = 0.0f;
    
    [unroll]
    for (int i = 0; i < 3; i++)
    {
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#else
        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#endif
        Offset++;
    }
    
    return Result;
}

float3 Kernel_5(int2 TexCoords)
{
    static const float KERNEL[5] =
    {
        0.06136f,
        0.24477f,
        0.38774f,
        0.24477f,
        0.06136f,
    };
    
    int    Offset = -2;
    float3 Result = 0.0f;
    
    [unroll]
    for (int i = 0; i < 5; i++)
    {
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#else
        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#endif
        Offset++;
    }
    
    return Result;
}

float3 Kernel_7(int2 TexCoords)
{
    static const float KERNEL[7] =
    {
        0.106595f,
        0.140367f,
        0.165569f,
        0.174938f,
        0.165569f,
        0.140367f,
        0.106595f,
    };
    
    int    Offset = -3;
    float3 Result = 0.0f;
    
    [unroll]
    for (int i = 0; i < 7; i++)
    {
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#else
        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#endif
        Offset++;
    }
    
    return Result;
}

float3 Kernel_9(int2 TexCoords)
{
    static const float KERNEL[9] =
    {
        0.063327f,
        0.093095f,
        0.122589f,
        0.144599f,
        0.152781f,
        0.144599f,
        0.122589f,
        0.093095f,
        0.063327f,
    };
    
    int    Offset = -4;
    float3 Result = 0.0f;
    
    [unroll]
    for (int i = 0; i < 9; i++)
    {
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#else
        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#endif
        Offset++;
    }
    
    return Result;
}

float3 Kernel_11(int2 TexCoords)
{
    static const float KERNEL[11] =
    {
        0.035822f,
        0.058790f,
        0.086425f,
        0.113806f,
        0.134240f,
        0.141836f,
        0.134240f,
        0.113806f,
        0.086425f,
        0.058790f,
        0.035822f,
    };
    
    int    Offset = -5;
    float3 Result = 0.0f;
    
    [unroll]
    for (int i = 0; i < 11; i++)
    {
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#else
        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#endif
        Offset++;
    }
    
    return Result;
}

float3 Kernel_13(int2 TexCoords)
{
    static const float KERNEL[13] =
    {
        0.018816f,
        0.034474f,
        0.056577f,
        0.083173f,
        0.109523f,
        0.129188f,
        0.136498f,
        0.129188f,
        0.109523f,
        0.083173f,
        0.056577f,
        0.034474f,
        0.018816f,
    };
    
    int    Offset = -6;
    float3 Result = 0.0f;
    
    [unroll]
    for (int i = 0; i < 13; i++)
    {
#ifdef HORIZONTAL_PASS
        const int2 CurrentTexCoord = int2(TexCoords.x + Offset, TexCoords.y);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#else
        const int2 CurrentTexCoord = int2(TexCoords.x, TexCoords.y + Offset);
        Result += Texture[CurrentTexCoord].rgb * KERNEL[i];
#endif
        Offset++;
    }
    
    return Result;
}

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
    const int2 TexCoords = int2(Input.DispatchThreadID.xy);
    if (TexCoords.x > ScreenSize.x || TexCoords.y > ScreenSize.y)
    {
        return;
    }

    const float MaxKernelSize = 13.0f;
    float4 Sample   = Texture[TexCoords];
    float  Variance = Sample.a;
    
    float3 Result  = 0.0f;
    int KernelSize = int(trunc(clamp(1.0f + Variance * (MaxKernelSize - 1.0f), 1.0f, MaxKernelSize)));
    if (KernelSize < 3)
    {
        Result = Sample.rgb;
    }
    else if (KernelSize < 5)
    {
        Result = Kernel_3(TexCoords);
    }
    else if (KernelSize < 7)
    {
        Result = Kernel_5(TexCoords);
    }
    else if (KernelSize < 9)
    {
        Result = Kernel_7(TexCoords);
    }
    else if (KernelSize < 11)
    {
        Result = Kernel_9(TexCoords);
    }
    else if (KernelSize < 13)
    {
        Result = Kernel_11(TexCoords);
    }
    else
    {
        Result = Kernel_13(TexCoords);
    }
    
    Texture[TexCoords] = float4(Result, Variance);
}