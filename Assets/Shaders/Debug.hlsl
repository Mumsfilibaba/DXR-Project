#include "Structs.hlsli"
#include "Constants.hlsli"

ConstantBuffer<FCamera> CameraBuffer : register(b0);

// AABB Debug
#if AABB_DEBUG // NOTE: We need this define since the shader constant-block otherwise causes issues when compiling SPIR-V code

SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 WorldMatrix;
    float4   Color;
SHADER_CONSTANT_BLOCK_END

struct FVSInput
{
    float3 Position : POSITION0;
};

float4 AABB_VSMain(FVSInput Input) : SV_Position
{
    return mul(mul(float4(Input.Position, 1.0), Constants.WorldMatrix), CameraBuffer.ViewProjection);
}

float4 AABB_PSMain() : SV_Target
{
    const float3 Color = Constants.Color.rgb;
    return float4(Color, 1.0);
}

#endif // AABB_DEBUG

// PointLight Debug
#if POINTLIGHT_DEBUG // NOTE: We need this define since the shader constant-block otherwise causes issues when compiling SPIR-V code

SHADER_CONSTANT_BLOCK_BEGIN
    float4 Color;
    float3 WorldPosition;
    float  Padding;
SHADER_CONSTANT_BLOCK_END

struct FVSInput
{
    float3 Position : POSITION0;
};

float4 Light_VSMain(FVSInput Input) : SV_Position
{
    float3 Pos = Input.Position + Constants.WorldPosition;
    return mul(float4(Pos, 1.0), CameraBuffer.ViewProjection);
}

float4 Light_PSMain() : SV_Target
{
    return float4(Constants.Color.rgb, 1.0);
}

#endif // POINTLIGHT_DEBUG

// AABB Solid Debug
#if AABB_SOLID_DEBUG // NOTE: We need this define since the shader constant-block otherwise causes issues when compiling SPIR-V code

SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 WorldMatrix;
    float4   Color;
SHADER_CONSTANT_BLOCK_END

struct FVSInput
{
    float3 Position : POSITION0;
};

float4 AABBSolidDebug_VSMain(FVSInput Input) : SV_Position
{
    return mul(mul(float4(Input.Position, 1.0), Constants.WorldMatrix), CameraBuffer.ViewProjection);
}

float4 AABBSolidDebug_PSMain() : SV_Target
{
    return float4(Constants.Color);
}

#endif // AABB_SOLID_DEBUG

// LightProbe Debug
#if LIGHTPROBE_DEBUG // NOTE: We need this define since the shader constant-block otherwise causes issues when compiling SPIR-V code

TextureCube<float4> CubeMap : register(t0);
SamplerState CubeMapSampler : register(s0);

SHADER_CONSTANT_BLOCK_BEGIN
    float3 WorldPosition;
    float  Padding;
SHADER_CONSTANT_BLOCK_END

struct FVSInput
{
    float3 Position : POSITION0;
};

struct FVSOutput
{
    float3 Normal     : NORMAL0;
    float3 PositionWS : POSITION0;
    float4 Position   : SV_Position;
};

FVSOutput Probe_VSMain(FVSInput Input)
{
    FVSOutput Output;
    Output.Normal     = normalize(Input.Position);
    Output.PositionWS = Input.Position + Constants.WorldPosition;
    Output.Position   = mul(float4(Output.PositionWS, 1.0), CameraBuffer.ViewProjection);
    return Output;
}

struct FPSInput
{
    float3 Normal     : NORMAL0;
    float3 PositionWS : POSITION0;
};

float4 Probe_PSMain(FPSInput Input) : SV_Target
{
    const float3 ViewWS     = normalize(CameraBuffer.PositionWS - Input.PositionWS);
    const float3 NormalWS   = normalize(Input.Normal);
    const float3 Reflection = reflect(-ViewWS, NormalWS);
    
    const float3 CubeMapSample = CubeMap.SampleLevel(CubeMapSampler, Reflection, 0.0).rgb;
    return float4(CubeMapSample, 1.0);
}

#endif // LIGHTPROBE_DEBUG
