#include "Constants.hlsli"
#include "Structs.hlsli"

struct SPerCascade
{
    int CascadeIndex;
};

struct SPerObject
{
    float4x4 ModelMatrix;
};

// PerObject
ConstantBuffer<SPerObject> PerObjectBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);

ConstantBuffer<SPerCascade> PerCascadeBuffer : register(b0);

StructuredBuffer<SCascadeMatrices> CascadeMatrixBuffer : register(t0);

// VS
struct VSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct GSOutput
{
    float3 Position : POSITION0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

///////////////////////////////
// Cascade Shadow Generation //
///////////////////////////////

float4 Cascade_VSMain(VSInput Input) : SV_POSITION
{
    float4x4 LightViewProjection = CascadeMatrixBuffer[PerCascadeBuffer.CascadeIndex].ViewProj;
    
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), PerObjectBuffer.ModelMatrix);
    return mul(WorldPosition, LightViewProjection);
}

void Cascade_GSMain(triangle float4 InPosition[3], inout TriangleStream<GSOutput> OutStream)
{
}

///////////////////////////////////
// Point Light Shadow Generation //
///////////////////////////////////

cbuffer LightBuffer : register(b0, space0)
{
    float4x4 LightProjection;
    float3 LightPosition;
    float  LightFarPlane;
}

struct VSOutput
{
    float3 WorldPosition : POSITION0;
    float4 Position      : SV_POSITION;
};

VSOutput Point_VSMain(VSInput Input)
{
    VSOutput Output = (VSOutput)0;
    
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), PerObjectBuffer.ModelMatrix);
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

////////////////////////////////
// Variance Shadow Generation //
////////////////////////////////

float4 VSM_VSMain(VSInput Input) : SV_Position
{
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), PerObjectBuffer.ModelMatrix);
    return mul(WorldPosition, LightProjection);
}

float4 VSM_PSMain(float4 Position : SV_Position) : SV_Target0
{
    float Depth = Position.z;
    return float4(Depth, Depth * Depth, 0.0f, 1.0f);
}