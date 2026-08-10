#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "TransformHelpers.hlsli"
#include "Constants.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "FastMath.hlsli"
#include "ParallaxMapping.hlsli"
#include "TangentSpace.hlsli"

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_NORMAL_MAPPING
    #define ENABLE_NORMAL_MAPPING (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

#ifndef ENABLE_DOUBLE_SIDED
    #define ENABLE_DOUBLE_SIDED (1)
#endif

#define MATERIAL_SRV_REGISTER_BASE 0
#define MATERIAL_ARRAY_REGISTER    t7
#define MATERIAL_SAMPLE_NORMAL     ENABLE_NORMAL_MAPPING
#include "MaterialSampling.hlsli"

ConstantBuffer<FCamera>    CameraBuffer    : register(b0);
ConstantBuffer<FPerObject> PerObjectBuffer : register(b1);

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    float SpecularAAStrength;
    float SpecularAAMaxRoughnessGain;
    float Padding0;
    float Padding1;
SHADER_CONSTANT_BLOCK_END

// ------------------------------------------------------------------------------------------------
// VertexShader
// ------------------------------------------------------------------------------------------------

struct FVSInput
{
    float3 Position : POSITION0;
    float4 Normal   : NORMAL0;
    float4 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct FVSOutput
{
    float3 Normal           : NORMAL0;
    float4 Tangent          : TANGENT0;
    float2 TexCoord	        : TEXCOORD0;
    float3 PositionWS       : POSITION0;
    float4 ClipPosition     : POSITION1;
    float4 PrevClipPosition : POSITION2;
    float4 Position         : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    // Position
    const float3 PositionWS3 = TransformPositionWS(PerObjectBuffer, Input.Position);
    const float4 PositionWS  = float4(PositionWS3, 1.0);

    const float3 Normal  = TransformDirectionInvT(PerObjectBuffer, Input.Normal.xyz);
    const float3 Tangent = TransformDirectionWS(PerObjectBuffer, Input.Tangent.xyz);

    FVSOutput Output;
    Output.Normal           = Normal;
    Output.Tangent          = float4(Tangent, Input.Tangent.w * PerObjectBuffer.DeterminantSign);
    Output.Position         = mul(PositionWS, CameraBuffer.ViewProjection);
    Output.PositionWS       = PositionWS3;
    Output.ClipPosition     = Output.Position;
    // TODO: Handle moving objects (aka PrevTransform)
    Output.PrevClipPosition = mul(PositionWS, CameraBuffer.PrevViewProjection);
    Output.TexCoord         = Input.TexCoord;
    return Output;
}

// ------------------------------------------------------------------------------------------------
// PixelShader
// ------------------------------------------------------------------------------------------------

struct FPSInput
{
    float3 Normal           : NORMAL0;
    float4 Tangent          : TANGENT0;
    float2 TexCoord         : TEXCOORD0;
    float3 PositionWS       : POSITION0;
    float4 ClipPosition     : POSITION1;
    float4 PrevClipPosition : POSITION2;
    float4 Position         : SV_Position;
    bool   bIsFrontFace     : SV_IsFrontFace;
};

struct FPSOutput
{
    float4 Albedo   : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 Material : SV_Target2;
    float2 Velocity : SV_Target3;
};

FPSOutput PSMain(FPSInput Input)
{
    const FMaterial MaterialData = Materials[PerObjectBuffer.MaterialIndex];

    float2 TexCoords     = Input.TexCoord;
    float3 SurfaceNormal = Input.Normal;
    float  TangentSign   = Input.Tangent.w;

#if ENABLE_DOUBLE_SIDED
    if (!Input.bIsFrontFace)
    {
        SurfaceNormal = -SurfaceNormal;
        TangentSign   = -TangentSign;
    }
#endif

#if ENABLE_PARALLAX_MAPPING
    const float2   TexCoordsDx    = ddx(TexCoords);
    const float2   TexCoordsDy    = ddy(TexCoords);
    const float3x3 WorldToTangent = CreateWorldToTangent(SurfaceNormal, Input.Tangent.xyz, TangentSign);
    const float3   ViewDir        = normalize(mul(WorldToTangent, CameraBuffer.PositionWS - Input.PositionWS));

    bool bParallaxDiscard = false;
    TexCoords = ApplyMaterialParallax(MaterialData, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, bParallaxDiscard);
    #if ENABLE_PARALLAX_CLIPPING
        if (bParallaxDiscard)
        {
            discard;
        }
    #endif
#endif

    const FMaterialSurface Surface = SampleMaterialSurface(MaterialData, TexCoords);

#if ENABLE_ALPHA_MASK
    [[branch]]
    if (Surface.Opacity < 0.5)
    {
        discard;
    }
#endif

    float3 Albedo = Surface.BaseColor;

    // Sample normal
#if ENABLE_NORMAL_MAPPING
    float3 Normal = DecodeTangentNormal(Surface.NormalTS, SurfaceNormal, Input.Tangent.xyz, TangentSign);
#else
    float3 Normal = normalize(SurfaceNormal);
#endif

    // Pack the normal and prepare for output
    Normal = PackNormal(Normal);

    const float Occlusion = Surface.Occlusion;
    const float Metallic  = Surface.Metallic;
    float       Roughness = Surface.Roughness;

    Roughness = FilterRoughnessGeometric(Roughness, Normal, Constants.SpecularAAStrength, Constants.SpecularAAMaxRoughnessGain);

    // Velocity
    float3 PositionNDC     = (Input.ClipPosition.xyz / Input.ClipPosition.w);
    float3 PrevPositionNDC = (Input.PrevClipPosition.xyz / Input.PrevClipPosition.w);
    float2 Velocity        = (PositionNDC.xy - CameraBuffer.ProjectionJitter) - (PrevPositionNDC.xy - CameraBuffer.PrevProjectionJitter);

    // Output
    FPSOutput Output;
    Output.Albedo   = float4(Albedo, 1.0);
    Output.Normal   = float4(Normal, 1.0);
    Output.Material = float4(Roughness, Metallic, Occlusion, 1.0);
    Output.Velocity = Velocity;

    return Output;
}
