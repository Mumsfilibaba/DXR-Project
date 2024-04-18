#include "Constants.hlsli"
#include "Structs.hlsli"

#ifndef ENABLE_PACKED_MATERIAL_TEXTURE
    #define ENABLE_PACKED_MATERIAL_TEXTURE (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
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

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    ConstantBuffer<FMaterial> MaterialBuffer : register(b1);
    SamplerState MaterialSampler : register(s0);

#if ENABLE_ALPHA_MASK
#if ENABLE_PACKED_MATERIAL_TEXTURE
    Texture2D<float4> AlphaMaskTex : register(t0);
#else
    Texture2D<float> AlphaMaskTex : register(t0);
#endif
#endif

#if ENABLE_PARALLAX_MAPPING
    Texture2D<float> HeightMap : register(t1);
#endif
#endif

// Cascade Shadow Generation

// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif
};

struct FVSCascadeOutput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
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
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif
    float3 Position : POSITION0;
};

void Cascade_GSMain(triangle float4 InPosition[3], inout TriangleStream<FGSOutput> OutStream)
{
}

// PixelShader

struct FPSCascadeInput
{
    float2 TexCoord : TEXCOORD0;
};

void Cascade_PSMain(FPSCascadeInput Input)
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;

#if ENABLE_ALPHA_MASK
#if ENABLE_PACKED_MATERIAL_TEXTURE
    const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).a;
#else
    const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).r;        
#endif
    [[branch]]
    if (AlphaMask < 0.5f)
    {
        discard;
    }
#endif
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
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif
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

struct FPSPointInput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif
    float3 WorldPosition : POSITION0;
};

float Point_PSMain(FPSPointInput Input) : SV_DepthLessEqual
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;

#if ENABLE_ALPHA_MASK
#if ENABLE_PACKED_MATERIAL_TEXTURE
    const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).a;
#else
    const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords);        
#endif
    [[branch]]
    if (AlphaMask < 0.5f)
    {
        // TODO: For some reason this makes the whole primitive be discarded, this needs further investigation
        // discard;
    }
#endif
#endif

    const float LightDistance = length(Input.WorldPosition.xyz - LightPosition) / LightFarPlane;
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