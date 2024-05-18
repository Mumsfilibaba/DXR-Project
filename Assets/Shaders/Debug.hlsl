#include "Structs.hlsli"
#include "Constants.hlsli"

ConstantBuffer<FCamera> CameraBuffer : register(b0);

// AABB Debug
#if AABB_DEBUG // NOTE: We need this define since the shader constant-block otherwise causes issues when compiling SPIR-V code

SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 TransformMat;
SHADER_CONSTANT_BLOCK_END

struct FVSInput
{
    float3 Position : POSITION0;
};

float4 AABB_VSMain(FVSInput Input) : SV_Position
{
    return mul(mul(float4(Input.Position, 1.0f), Constants.TransformMat), CameraBuffer.ViewProjection);
}

float4 AABB_PSMain() : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

#endif

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
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

float4 Light_VSMain(FVSInput Input) : SV_Position
{
    float3 Pos = Input.Position + Constants.WorldPosition;
    return mul(float4(Pos, 1.0f), CameraBuffer.ViewProjection);
}

float4 Light_PSMain() : SV_Target
{
    return float4(Constants.Color.rgb, 1.0f);
}

#endif

// Occlusion Volume Debug
#if OCCLUSION_VOLUME_DEBUG // NOTE: We need this define since the shader constant-block otherwise causes issues when compiling SPIR-V code

SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 TransformMat;
    float4   Color;
SHADER_CONSTANT_BLOCK_END

struct FVSInput
{
    float3 Position : POSITION0;
};

float4 OcclusionDebug_VSMain(FVSInput Input) : SV_Position
{
    return mul(mul(float4(Input.Position, 1.0f), Constants.TransformMat), CameraBuffer.ViewProjection);
}

float4 OcclusionDebug_PSMain() : SV_Target
{
    return float4(Constants.Color);
}

#endif
