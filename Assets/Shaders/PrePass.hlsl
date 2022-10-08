#include "Structs.hlsli"
#include "Constants.hlsli"

// PerObject Constants
cbuffer TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    float4x4 TransformMat;
};

// PerFrame
ConstantBuffer<FCamera>   CameraBuffer   : register(b0, space0);
ConstantBuffer<FMaterial> MaterialBuffer : register(b1, space0);

// PerObject Samplers
SamplerState MaterialSampler : register(s0);

Texture2D<float4> AlphaMaskTex : register(t0);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct FVSOutput
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output = (FVSOutput)0;
    Output.Position = mul(float4(Input.Position, 1.0f), TransformMat);
    Output.Position = mul(Output.Position, CameraBuffer.ViewProjection); 
    return Output;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
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
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;
    
    if (MaterialBuffer.EnableMask == ALPHA_ENABLED)
    {
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).r;
        if (AlphaMask < 0.5f)
        {
            discard;
        }
    }
    else if (MaterialBuffer.EnableMask == ALPHA_DIFFUSE_COMBINED)
    {
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).a;
        if (AlphaMask < 0.5f)
        {
            discard;
        }
    }
}