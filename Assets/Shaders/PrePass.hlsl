#include "Structs.hlsli"
#include "Constants.hlsli"

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

// PerObject Constants
SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 TransformMat;
SHADER_CONSTANT_BLOCK_END

// Per Frame
ConstantBuffer<FCamera> CameraBuffer : register(b0);

// Per Object
#if ENABLE_ALPHA_MASK
    ConstantBuffer<FMaterial> MaterialBuffer : register(b1);
    
    SamplerState MaterialSampler : register(s0);
    
    Texture2D<float4> AlphaMaskTex : register(t0);
#endif

// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;
#if ENABLE_ALPHA_MASK
    float2 TexCoord : TEXCOORD0;
#endif
};

struct FVSOutput
{
#if ENABLE_ALPHA_MASK 
    float2 TexCoord : TEXCOORD0;
#endif
    float4 Position : SV_POSITION;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output = (FVSOutput)0;
#if ENABLE_ALPHA_MASK 
    Output.TexCoord = Input.TexCoord;
#endif
    Output.Position = mul(float4(Input.Position, 1.0f), Constants.TransformMat);
    Output.Position = mul(Output.Position, CameraBuffer.ViewProjection); 
    return Output;
}

// PixelShader

struct FPSInput
{
    float2 TexCoord : TEXCOORD0;
};

#define ALPHA_DISABLED         (0)
#define ALPHA_ENABLED          (1)
#define ALPHA_DIFFUSE_COMBINED (2)

void PSMain(FPSInput Input)
{
#if ENABLE_ALPHA_MASK
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;
    
    [[branch]]
    if (MaterialBuffer.EnableMask == ALPHA_ENABLED)
    {
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).r;
        
        [[branch]]
        if (AlphaMask < 0.5f)
        {
            discard;
        }
    }
    else if (MaterialBuffer.EnableMask == ALPHA_DIFFUSE_COMBINED)
    {
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).a;
        
        [[branch]]
        if (AlphaMask < 0.5f)
        {
            discard;
        }
    }
#endif
}