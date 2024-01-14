#include "Constants.hlsli"
#include "Structs.hlsli"

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

struct FPerCascade
{
    int CascadeIndex;
    int Padding0;
    int Padding1;
    int Padding2;
};

// PerObject
SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 ModelMatrix;
SHADER_CONSTANT_BLOCK_END


#if SHADER_LANG == SHADER_LANG_MSL
    ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b2);
#else
    ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b0);
#endif

StructuredBuffer<FCascadeMatrices> CascadeMatrixBuffer : register(t0);

#if ENABLE_ALPHA_MASK
    ConstantBuffer<FMaterial> MaterialBuffer : register(b1);
    
    SamplerState MaterialSampler : register(s0);
    
    Texture2D<float4> AlphaMaskTex : register(t1);
#endif

// Cascade Shadow Generation

// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;
#if ENABLE_ALPHA_MASK
    float2 TexCoord : TEXCOORD0;
#endif
};

struct FVSCascadeOutput
{
#if ENABLE_ALPHA_MASK 
    float2 TexCoord : TEXCOORD0;
#endif
    float4 Position : SV_POSITION;
};

FVSCascadeOutput Cascade_VSMain(FVSInput Input)
{
    const int      CascadeIndex        = PerCascadeBuffer.CascadeIndex;
    const float4x4 LightViewProjection = CascadeMatrixBuffer[CascadeIndex].ViewProj;

    FVSCascadeOutput Output = (FVSCascadeOutput)0;
#if ENABLE_ALPHA_MASK 
    Output.TexCoord = Input.TexCoord;
#endif
    Output.Position = mul(float4(Input.Position, 1.0f), Constants.ModelMatrix);
    Output.Position = mul(Output.Position, LightViewProjection);
    return Output;
}

// GeometryShader

struct FGSOutput
{
#if ENABLE_ALPHA_MASK 
    float2 TexCoord : TEXCOORD0;
#endif
    float3 Position : POSITION0;
};

void Cascade_GSMain(triangle float4 InPosition[3], inout TriangleStream<FGSOutput> OutStream)
{
}

// PixelShader

struct FPSInput
{
    float2 TexCoord : TEXCOORD0;
};

#define ALPHA_DISABLED         (0)
#define ALPHA_ENABLED          (1)
#define ALPHA_DIFFUSE_COMBINED (2)

void Cascade_PSMain(FPSInput Input)
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

// Point Light Shadow Generation

cbuffer LightBuffer : register(b0)
{
    float4x4 LightProjection;
    float3   LightPosition;
    float    LightFarPlane;
}

struct FVSPointOutput
{
    float3 WorldPosition : POSITION0;
    float4 Position      : SV_POSITION;
};

FVSPointOutput Point_VSMain(FVSInput Input)
{
    FVSPointOutput Output = (FVSPointOutput)0;
    Output.WorldPosition = mul(float4(Input.Position, 1.0f), Constants.ModelMatrix).xyz;
    Output.Position      = mul(float4(Output.WorldPosition, 1.0f), LightProjection);
    return Output;
}

float Point_PSMain(float3 WorldPosition : POSITION0) : SV_Depth
{
    float LightDistance = length(WorldPosition.xyz - LightPosition);
    LightDistance       = LightDistance / LightFarPlane;
    return LightDistance;
}

// Variance Shadow Generation

float4 VSM_VSMain(FVSInput Input) : SV_Position
{
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), Constants.ModelMatrix);
    return mul(WorldPosition, LightProjection);
}

float4 VSM_PSMain(float4 Position : SV_Position) : SV_Target0
{
    float Depth = Position.z;
    return float4(Depth, Depth * Depth, 0.0f, 1.0f);
}