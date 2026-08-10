#include "PBRHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "TransformHelpers.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "ImageBasedLighting.hlsli"
#include "Shadows/CascadeStructs.hlsli"
#include "Shadows/ShadowHelpers.hlsli"
#include "ParallaxMapping.hlsli"
#include "TangentSpace.hlsli"

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

#ifndef ENABLE_TRANSLUCENT
    #define ENABLE_TRANSLUCENT (0)
#endif

#ifndef ENABLE_REFRACTION
    #define ENABLE_REFRACTION (0)
#endif

#ifndef MAX_LIGHTS_PER_TILE
    #define MAX_LIGHTS_PER_TILE 1024
#endif

#define BASE_OCCLUSION 0.1

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    int NumPointLights;
    int NumShadowCastingPointLights;
    int NumSkyLightMips;
    int NumLightProbes;
    // 16-32
    int   bEnablePointLightShadows;
    float IndirectSpecularStrength;
    float SpecularAAStrength;
    float SpecularAAMaxRoughnessGain;
    // 32-48
    int   bEnableSunShadows;
    float ShadowFilterSize;
    float ShadowMaxFilterSize;
    uint  ShadowMapSize;
    // 48-64
    uint  ShadowNumSamples;
SHADER_CONSTANT_BLOCK_END

ConstantBuffer<FCamera> CameraBuffer : register(b0);

cbuffer PointLightsBuffer : register(b1)
{
    FPointLight PointLights[MAX_LIGHTS_PER_TILE];
}

cbuffer PointLightsPosRadBuffer : register(b2)
{
    FPositionRadius PointLightsPosRad[MAX_LIGHTS_PER_TILE];
}

cbuffer ShadowCastingPointLightsBuffer : register(b3)
{
    FShadowPointLight ShadowCastingPointLights[8];
}

cbuffer ShadowCastingPointLightsPosRadBuffer : register(b4)
{
    FPositionRadius ShadowCastingPointLightsPosRad[8];
}

ConstantBuffer<FDirectionalLight> DirLightBuffer       : register(b5);
ConstantBuffer<FPerObject>        PerObjectBuffer      : register(b6);
ConstantBuffer<FLightProbeInfo>   LightProbeInfoBuffer : register(b7);

#define MATERIAL_SRV_REGISTER_BASE 5
#define MATERIAL_ARRAY_REGISTER    t12
#include "MaterialSampling.hlsli"

#define SHADOW_FILTER_FUNCTION SHADOW_FILTER_FUNCTION_POISSON_DISK

#define CASCADE_SHADOW_SPLITS_REGISTER             t16
#define CASCADE_SHADOW_CASCADES_REGISTER           t3
#define CASCADE_SHADOW_SAMPLER_POINT_CMP_REGISTER  s4
#define CASCADE_SHADOW_SAMPLER_LINEAR_CMP_REGISTER s5
#define CASCADE_SHADOW_SAMPLER_POINT_REGISTER      s6
#include "Shadows/CascadeShadowSampling.hlsli"

SamplerState           LUTSampler        : register(s1);
SamplerState           IrradianceSampler : register(s2);
SamplerComparisonState ShadowMapSampler0 : register(s3);

TextureCube<float4>     IrradianceMap         : register(t0);
TextureCube<float4>     SpecularIrradianceMap : register(t1);
Texture2D<float2>       IntegrationLUT        : register(t2);
TextureCubeArray<float> PointLightShadowMaps  : register(t4);
TextureCube<float4>     SkyboxCubeMap         : register(t13);
TextureCube<float4>     ProbeDiffuseCubeMap   : register(t14);
TextureCube<float4>     ProbeSpecularCubeMap  : register(t15);

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
    float3 WorldPosition : POSITION0;
    float3 Normal        : NORMAL0;
    float4 Tangent       : TANGENT0;
    float2 TexCoord      : TEXCOORD0;
    float4 Position      : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;
    Output.Normal   = TransformDirectionInvT(PerObjectBuffer, Input.Normal.xyz);
    Output.Tangent  = float4(TransformDirectionWS(PerObjectBuffer, Input.Tangent.xyz), Input.Tangent.w * PerObjectBuffer.DeterminantSign);
    Output.TexCoord = Input.TexCoord;

    const float3 WorldPosition3 = TransformPositionWS(PerObjectBuffer, Input.Position);
    Output.Position      = mul(float4(WorldPosition3, 1.0), CameraBuffer.ViewProjection);
    Output.WorldPosition = WorldPosition3;

    return Output;
}

// ------------------------------------------------------------------------------------------------
// PixelShader
// ------------------------------------------------------------------------------------------------

struct FPSInput
{
    float3 WorldPosition : POSITION0;
    float3 Normal        : NORMAL0;
    float4 Tangent       : TANGENT0;
    float2 TexCoord      : TEXCOORD0;
    float4 Position      : SV_Position;
    bool   bIsFrontFace  : SV_IsFrontFace;
};

float4 PSMain(FPSInput Input) : SV_Target0
{
    const FMaterial MaterialData = Materials[PerObjectBuffer.MaterialIndex];

    float2 TexCoords     = Input.TexCoord;
    float3 SurfaceNormal = Input.Normal;
    float  TangentSign   = Input.Tangent.w;

    if (!Input.bIsFrontFace)
    {
        SurfaceNormal = -SurfaceNormal;
        TangentSign   = -TangentSign;
    }

#if ENABLE_PARALLAX_MAPPING
    {
        const float2   TexCoordsDx    = ddx(TexCoords);
        const float2   TexCoordsDy    = ddy(TexCoords);
        const float3x3 WorldToTangent = CreateWorldToTangent(SurfaceNormal, Input.Tangent.xyz, TangentSign);
        const float3   ViewDir        = normalize(mul(WorldToTangent, CameraBuffer.PositionWS - Input.WorldPosition));

        bool bParallaxDiscard = false;
        TexCoords = ApplyMaterialParallax(MaterialData, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, bParallaxDiscard);
    #if ENABLE_PARALLAX_CLIPPING
        if (bParallaxDiscard)
        {
            discard;
        }
    #endif
    }
#endif

    const FMaterialSurface Surface = SampleMaterialSurface(MaterialData, TexCoords);

#if ENABLE_ALPHA_MASK
    [branch]
    if (Surface.Opacity < 0.5)
    {
        discard;
    }
#endif

    const float3 SampledAlbedo   = Surface.BaseColor;
    const float3 WorldPosition   = Input.WorldPosition;
    const float3 V               = normalize(CameraBuffer.PositionWS - WorldPosition);
    const float3 N               = DecodeTangentNormal(Surface.NormalTS, SurfaceNormal, Input.Tangent.xyz, TangentSign);
    const float  SampledAO       = Surface.Occlusion;
    const float  SampledMetallic = Surface.Metallic;
    const float  Roughness       = FilterRoughnessGeometric(Surface.Roughness, PackNormal(N), Constants.SpecularAAStrength, Constants.SpecularAAMaxRoughnessGain);
    const float  Occlusion       = saturate(BASE_OCCLUSION + SampledAO);

    float3 F0 = 0.04;
    F0 = lerp(F0, SampledAlbedo, SampledMetallic);

    float3 L0 = 0.0;

    // Pointlights
    [loop]
    for (int i = 0; i < Constants.NumPointLights; i++)
    {
        const FPointLight     Light       = PointLights[i];
        const FPositionRadius LightPosRad = PointLightsPosRad[i];

        float3 L = LightPosRad.Position - WorldPosition;
        float DistanceSqrd = dot(L, L);
        float Attenuation  = 1.0 / max(DistanceSqrd, 0.01 * 0.01);
        L = normalize(L);

        float3 IncidentRadiance = Light.Color * Attenuation;
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, SampledAlbedo, Roughness, SampledMetallic);
            
        L0 += IncidentRadiance;
    }

    // Shadow-casting pointlights
    [loop]
    for (int j = 0; j < Constants.NumShadowCastingPointLights; j++)
    {
        const FShadowPointLight Light       = ShadowCastingPointLights[j];
        const FPositionRadius   LightPosRad = ShadowCastingPointLightsPosRad[j];

        float ShadowFactor;

        [branch]
        if (Constants.bEnablePointLightShadows)
        {
            ShadowFactor = PointLightShadowFactor(PointLightShadowMaps, float(j), ShadowMapSampler0, WorldPosition, N, Light, LightPosRad);
        }
        else
        {
            ShadowFactor = 1.0;
        }

        [branch]
        if (ShadowFactor > 0.0)
        {
            float3 L            = LightPosRad.Position - WorldPosition;
            float  DistanceSqrd = dot(L, L);
            float  Attenuation  = 1.0 / max(DistanceSqrd, 0.01 * 0.01);
            L = normalize(L);
            
            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, SampledAlbedo, Roughness, SampledMetallic);
            
            L0 += IncidentRadiance * ShadowFactor;
        }
    }

    // DirectionalLights
    float ShadowMask = 1.0;

    {
        const FDirectionalLight Light = DirLightBuffer;

        [branch]
        if (Constants.bEnableSunShadows)
        {
            FCascadeShadowContext ShadowContext;
            ShadowContext.Light                  = Light;
            ShadowContext.Settings.FilterSize    = Constants.ShadowFilterSize;
            ShadowContext.Settings.MaxFilterSize = Constants.ShadowMaxFilterSize;
            ShadowContext.Settings.ShadowMapSize = Constants.ShadowMapSize;
            ShadowContext.Settings.NumSamples    = Constants.ShadowNumSamples;

            const uint2 Pixel = uint2(Input.Position.xy);

            uint RandomSeed   = InitRandom(Pixel, CameraBuffer.ViewportWidth, 0);
            uint CascadeIndex = 0;

            const float ViewPosZ = Depth_ProjToView(Input.Position.z, CameraBuffer.ProjectionInv);
            ShadowMask = ComputeCascadeShadow(ShadowContext, WorldPosition, N, ViewPosZ, CascadeIndex, RandomSeed);
        }

        [branch]
        if (ShadowMask > 0.0)
        {
            const float3 L = normalize(-Light.Direction);

            float3 IncidentRadiance = Light.Color;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, SampledAlbedo, Roughness, SampledMetallic);

            L0 += IncidentRadiance * ShadowMask;
        }
    }

    ShadowMask = max(0.7, ShadowMask);

    // Image Based Lightning
    float3 FinalColor = L0;

    {
        const float  NDotV      = max(dot(N, V), 0.0);
        const float3 Reflection = reflect(-V, N);

        float3 F  = FresnelSchlick_Roughness(F0, V, N, Roughness);
        float3 Ks = F;
        float3 Kd = (1.0 - Ks) * (1.0 - SampledMetallic);

        FDiffuseEnvironmentInfo DiffuseEnvironmentInfo;
        DiffuseEnvironmentInfo.NormalUVW = N;

        FSpecularEnvironmentInfo SpecularEnvironmentInfo;
        SpecularEnvironmentInfo.Roughness = Roughness;

        float3 SpecularSample;
        float3 DiffuseSample;

        // Same probe-versus-skylight choice the deferred light pass makes
        [branch]
        if (Constants.NumLightProbes > 0 && IsInsideAABB(WorldPosition, LightProbeInfoBuffer.BoxMinWS, LightProbeInfoBuffer.BoxMaxWS))
        {
            FBoxProjectionInfo BoxProjectionInfo;
            BoxProjectionInfo.ReflectionUVW     = normalize(Reflection);
            BoxProjectionInfo.PositionWS        = WorldPosition;
            BoxProjectionInfo.CubeMapPositionWS = LightProbeInfoBuffer.BoxOriginWS;
            BoxProjectionInfo.BoxMinWS          = LightProbeInfoBuffer.BoxMinWS;
            BoxProjectionInfo.BoxMaxWS          = LightProbeInfoBuffer.BoxMaxWS;
            BoxProjectionInfo.BoxProjection     = LightProbeInfoBuffer.BoxProjection;

            SpecularEnvironmentInfo.ReflectionUVW = BoxProjection(BoxProjectionInfo);

            // The probe is a local capture, so it already contains the local lighting and is not dimmed by the sun shadow
            SpecularSample = SpecularEnvironment(ProbeSpecularCubeMap, IrradianceSampler, SpecularEnvironmentInfo, Constants.NumSkyLightMips);
            DiffuseSample  = DiffuseEnvironment(ProbeDiffuseCubeMap, IrradianceSampler, DiffuseEnvironmentInfo);
        }
        else
        {
            SpecularEnvironmentInfo.ReflectionUVW = Reflection;

            SpecularSample = SpecularEnvironment(SpecularIrradianceMap, IrradianceSampler, SpecularEnvironmentInfo, Constants.NumSkyLightMips) * ShadowMask;
            DiffuseSample  = DiffuseEnvironment(IrradianceMap, IrradianceSampler, DiffuseEnvironmentInfo) * ShadowMask;
        }

        const float2 BRDFIntegration = GetIntegrationConstants(IntegrationLUT, LUTSampler, NDotV, Roughness);
        const float3 DiffuseColor    = lerp(SampledAlbedo * (1.0 - F0), float3(0.0, 0.0, 0.0), SampledMetallic);
        const float3 Specular        = SpecularSample * (F * BRDFIntegration.x + BRDFIntegration.y) * Constants.IndirectSpecularStrength;
        const float3 Diffuse         = DiffuseSample * DiffuseColor;
        const float3 Ambient         = (Kd * Diffuse + Specular) * Occlusion;

        FinalColor = Ambient + L0;
    }

    // Finalize
#if ENABLE_TRANSLUCENT
    const float Opacity = saturate(Surface.Opacity);

    float3 Transmitted = 0.0;
    float  Coverage    = Opacity;

    #if ENABLE_REFRACTION
        const float  Eta        = 1.0 / max(MaterialData.IndexOfRefraction, 1.0);
        const float3 RefractDir = refract(-V, N, Eta);
        const float  MipLevel   = Roughness * float(max(Constants.NumSkyLightMips - 1, 0));
        const float3 SampleDir  = all(RefractDir == 0.0) ? reflect(-V, N) : RefractDir;
        const float3 Sky        = SkyboxCubeMap.SampleLevel(IrradianceSampler, SampleDir, MipLevel).rgb;

        const float Transmission = (1.0 - Opacity) * MaterialData.RefractionStrength;
        Transmitted = Sky * Transmission;
        Coverage    = Opacity + Transmission;
    #endif

    return float4(FinalColor * Opacity + Transmitted, Coverage);
#else
    return float4(FinalColor, Luminance(FinalColor));
#endif
}
