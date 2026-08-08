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

#if ENABLE_BINDLESS
    #include "MaterialBindless.hlsli"
#endif

#define MATERIAL_ARRAY_REGISTER t4
#include "MaterialArray.hlsli"

ConstantBuffer<FCamera>    CameraBuffer    : register(b0);
ConstantBuffer<FPerObject> PerObjectBuffer : register(b1);

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    float SpecularAAStrength;
    float SpecularAAMaxRoughnessGain;
    float Padding0;
    float Padding1;
SHADER_CONSTANT_BLOCK_END

#if !ENABLE_BINDLESS
    SamplerState MaterialSampler : register(s0);

    // Unified per-material texture layout: Albedo (RGBA, t0), Normal (t1),
    // Material (R=AO, G=Roughness, B=Metallic, t2), Height (t3).
    Texture2D<float4> AlbedoMap   : register(t0);
    Texture2D<float3> MaterialMap : register(t2);
    
    #if ENABLE_NORMAL_MAPPING
        Texture2D<float3> NormalTex : register(t1);
    #endif
    #if ENABLE_PARALLAX_MAPPING
        Texture2D<float> HeightTex : register(t3);
    #endif
#endif

SamplerState GetMaterialSampler()
{
#if ENABLE_BINDLESS
    return GetMaterialSamplerBindless(Materials[PerObjectBuffer.MaterialIndex]);
#else
    return MaterialSampler;
#endif
}

float4 GetAlbedo(float2 TexCoord)
{
#if ENABLE_BINDLESS
    const FMaterial MaterialData = Materials[PerObjectBuffer.MaterialIndex];
    if (!IsAlbedoBindlessValid(MaterialData))
    {
        return float4(1.0, 1.0, 1.0, 1.0);
    }

    return GetAlbedoBindless(MaterialData).Sample(GetMaterialSampler(), TexCoord);
#else
    return AlbedoMap.Sample(MaterialSampler, TexCoord);
#endif
}

#if ENABLE_NORMAL_MAPPING
float3 GetNormal(float2 TexCoord)
{
    #if ENABLE_BINDLESS
        const FMaterial MaterialData = Materials[PerObjectBuffer.MaterialIndex];
        if (!IsNormalBindlessValid(MaterialData))
        {
            return float3(0.5, 0.5, 1.0);
        }

        return GetNormalBindless(MaterialData).Sample(GetMaterialSampler(), TexCoord);
    #else
        return NormalTex.Sample(MaterialSampler, TexCoord);
    #endif
}
#endif

float3 GetMaterialParams(float2 TexCoord)
{
#if ENABLE_BINDLESS
    const FMaterial MaterialData = Materials[PerObjectBuffer.MaterialIndex];
    if (!IsMaterialBindlessValid(MaterialData))
    {
        return float3(1.0, 1.0, 0.0);
    }

    return GetMaterialBindless(MaterialData).Sample(GetMaterialSampler(), TexCoord);
#else
    return MaterialMap.Sample(MaterialSampler, TexCoord);
#endif
}

#if ENABLE_PARALLAX_MAPPING
float2 ApplyParallax(FMaterial MaterialData, float2 TexCoords, float3 ViewDir, float2 TexCoordsDx, float2 TexCoordsDy, out bool bParallaxDiscard)
{
    bParallaxDiscard = false;

    #if ENABLE_BINDLESS
        if (!IsHeightBindlessValid(MaterialData))
        {
            return TexCoords;
        }

        return ParallaxMapUV(GetHeightBindless(MaterialData), GetMaterialSampler(), TexCoords, ViewDir, TexCoordsDx, TexCoordsDy,
            MaterialData.ParallaxHeightScale, MaterialData.ParallaxMinLayers, MaterialData.ParallaxMaxLayers,
            bParallaxDiscard);
    #else
        return ParallaxMapUV(HeightTex, MaterialSampler, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy,
            MaterialData.ParallaxHeightScale, MaterialData.ParallaxMinLayers, MaterialData.ParallaxMaxLayers,
            bParallaxDiscard);
    #endif
}
#endif

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
    TexCoords = ApplyParallax(MaterialData, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, bParallaxDiscard);
    #if ENABLE_PARALLAX_CLIPPING
        if (bParallaxDiscard)
        {
            discard;
        }
    #endif
#endif

    // Sample albedo (alpha in .a channel)
    const float4 AlbedoSample = GetAlbedo(TexCoords);

#if ENABLE_ALPHA_MASK
    [[branch]]
    if (AlbedoSample.a < 0.5)
    {
        discard;
    }
#endif

    float3 Albedo = SRGBToLinear(AlbedoSample.rgb) * MaterialData.Albedo;

    // Sample normal
#if ENABLE_NORMAL_MAPPING
    float3 SampledNormal = GetNormal(TexCoords);
    SampledNormal = UnpackNormalBC5(SampledNormal);
    SampledNormal = ApplyNormalMapAxis(SampledNormal, IsNormalMapPositiveY(MaterialData));

    float3 Normal = DecodeTangentNormal(SampledNormal, SurfaceNormal, Input.Tangent.xyz, TangentSign);
#else
    float3 Normal = normalize(SurfaceNormal);
#endif

    // Pack the normal and prepare for output
    Normal = PackNormal(Normal);

    // Sample material params from packed materialparam texture (R=AO, G=Roughness, B=Metallic)
    const float3 MaterialParams = GetMaterialParams(TexCoords);
    const float  Occlusion      = MaterialParams.r * MaterialData.AO;
    float        Roughness      = MaterialParams.g * MaterialData.Roughness;
    const float  Metallic       = MaterialParams.b * MaterialData.Metallic;

    // Specular anti-aliasing
    {
        const float Strength         = Constants.SpecularAAStrength;
        const float MaxRoughnessGain = Constants.SpecularAAMaxRoughnessGain;

        float  Roughness2         = Roughness * Roughness;
        float3 DnDu               = ddx(Normal);
        float3 DnDv               = ddy(Normal);
        float  Variance           = (dot(DnDu, DnDu) + dot(DnDv, DnDv));
        float  KernelRoughness2   = min(Variance * Strength, MaxRoughnessGain);
        float  FilteredRoughness2 = saturate(Roughness2 + KernelRoughness2);
        
        Roughness = FastSqrt(FilteredRoughness2);
    }

    // Ensure we do not go above or below a certain roughness threshold
    Roughness = min(max(Roughness, MIN_ROUGHNESS), MAX_ROUGHNESS);

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
