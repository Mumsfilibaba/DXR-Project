#include "Constants.hlsli"
#include "Structs.hlsli"

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
ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b1);
#else
ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b0);
#endif

StructuredBuffer<FCascadeMatrices> CascadeMatrixBuffer : register(t0);

// VS
struct FVSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct FGSOutput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

// Cascade Shadow Generation

float4 Cascade_VSMain(FVSInput Input) : SV_POSITION
{
    const int CascadeIndex = PerCascadeBuffer.CascadeIndex;
    float4x4 LightViewProjection = CascadeMatrixBuffer[CascadeIndex].ViewProj;
    
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), Constants.ModelMatrix);
    return mul(WorldPosition, LightViewProjection);
}

void Cascade_GSMain(triangle float4 InPosition[3], inout TriangleStream<FGSOutput> OutStream)
{
}

// Point Light Shadow Generation

cbuffer LightBuffer : register(b0)
{
    float4x4 LightProjection;
    float3   LightPosition;
    float    LightFarPlane;
}

struct FVSOutput
{
    float3 WorldPosition : POSITION0;
    float4 Position      : SV_POSITION;
};

FVSOutput Point_VSMain(FVSInput Input)
{
    FVSOutput Output = (FVSOutput)0;
    
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), Constants.ModelMatrix);
    Output.WorldPosition = WorldPosition.xyz;
    Output.Position      = mul(WorldPosition, LightProjection);
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