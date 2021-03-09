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
        0.071303f,
        0.131514f,
        0.189879f,
        0.214607f,
        0.189879f,
        0.131514f,
        0.071303f,
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
        0.028532f,
        0.067234f,
        0.124009f,
        0.179044f,
        0.202360f,
        0.179044f,
        0.124009f,
        0.067234f,
        0.028532f,
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
        0.009300f,
        0.028002f,
        0.065984f,
        0.121703f,
        0.175713f,
        0.198596f,
        0.175713f,
        0.121703f,
        0.065984f,
        0.028002f,
        0.009300f,
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
        0.002406f,
        0.009255f,
        0.027867f,
        0.065666f,
        0.121117f,
        0.174868f,
        0.197641f,
        0.174868f,
        0.121117f,
        0.065666f,
        0.027867f,
        0.009255f,
        0.002406f,
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

    float Variance = Texture[TexCoords].a;
    int KernelSize = int(trunc(clamp(3.0f + Variance * 10.0f, 3.0f, 13.0f)));
    if (KernelSize % 2 == 0)
    {
        KernelSize = clamp(KernelSize + 1, 3, 13);
    }
    
    float3 Result = 0.0f;
    if (KernelSize == 3)
    {
        Result = Kernel_3(TexCoords);
    }
    else if (KernelSize == 5)
    {
        Result = Kernel_5(TexCoords);
    }
    else if (KernelSize == 7)
    {
        Result = Kernel_7(TexCoords);
    }
    else if (KernelSize == 9)
    {
        Result = Kernel_9(TexCoords);
    }
    else if (KernelSize == 11)
    {
        Result = Kernel_13(TexCoords);
    }
    else if (KernelSize == 13)
    {
        Result = Kernel_13(TexCoords);
    }
    
    Texture[TexCoords] = float4(Result, Variance);
}