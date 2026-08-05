#include "PBRHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "TransformHelpers.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "Shadows/CascadeStructs.hlsli"
#include "Shadows/ShadowHelpers.hlsli"
#include "ParallaxMapping.hlsli"
#include "TangentSpace.hlsli"

#ifndef BINDLESS_FORWARD_PASS
    #define BINDLESS_FORWARD_PASS (0)
#endif

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#if BINDLESS_FORWARD_PASS
    #include "MaterialBindless.hlsli"
#endif

// Per Frame Buffers

// TODO: Fix this
//cbuffer Constants : register(b0)
//{
//    int NumPointLights;
//    int NumSkyLightMips;
//};

ConstantBuffer<FCamera> CameraBuffer : register(b0);

cbuffer PointLightsBuffer : register(b1)
{
    FPointLight PointLights[32];
}

cbuffer PointLightsPosRadBuffer : register(b2)
{
    FPositionRadius PointLightsPosRad[32];
}

cbuffer ShadowCastingPointLightsBuffer : register(b3)
{
    FShadowPointLight ShadowCastingPointLights[8];
}

cbuffer ShadowCastingPointLightsPosRadBuffer : register(b4)
{
    FPositionRadius ShadowCastingPointLightsPosRad[8];
}

ConstantBuffer<FDirectionalLight> DirLightBuffer  : register(b5);
ConstantBuffer<FPerObject>        PerObjectBuffer : register(b6);

#define MATERIAL_ARRAY_REGISTER t9
#include "MaterialArray.hlsli"

SamplerState           LUTSampler        : register(s1);
SamplerState           IrradianceSampler : register(s2);
SamplerComparisonState ShadowMapSampler0 : register(s3);
SamplerComparisonState ShadowMapSampler1 : register(s4);

TextureCube<float4>     IrradianceMap         : register(t0);
TextureCube<float4>     SpecularIrradianceMap : register(t1);
Texture2D<float4>       IntegrationLUT        : register(t2);
Texture2D<float>        DirLightShadowMaps    : register(t3);
TextureCubeArray<float> PointLightShadowMaps  : register(t4);

#if !BINDLESS_FORWARD_PASS
    // Per-material textures: Albedo (t5), Normal (t6), Material (R=AO, G=Roughness, B=Metallic, t7), Height (t8).
    SamplerState      MaterialSampler : register(s0);
    Texture2D<float4> AlbedoTex       : register(t5);
    Texture2D<float4> NormalTex       : register(t6);
    Texture2D<float3> MaterialMap     : register(t7);

    #if ENABLE_PARALLAX_MAPPING
        Texture2D<float> HeightMap : register(t8);
    #endif
#endif

// ------------------------------------------------------------------------------------------------
// VertexShader
// ------------------------------------------------------------------------------------------------

struct FVSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
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
    Output.Normal   = TransformDirectionInvT(PerObjectBuffer, Input.Normal);
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
    #if BINDLESS_FORWARD_PASS
        TexCoords = ParallaxMapUV(GetHeightBindless(MaterialData), GetMaterialSamplerBindless(MaterialData), TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialData.ParallaxHeightScale, MaterialData.ParallaxMinLayers, MaterialData.ParallaxMaxLayers, bParallaxDiscard);
    #else
        TexCoords = ParallaxMapUV(HeightMap, MaterialSampler, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialData.ParallaxHeightScale, MaterialData.ParallaxMinLayers, MaterialData.ParallaxMaxLayers, bParallaxDiscard);
    #endif
    #if ENABLE_PARALLAX_CLIPPING
        if (bParallaxDiscard)
        {
            discard;
        }
    #endif
    }
#endif

#if BINDLESS_FORWARD_PASS
    const float4 AlbedoSample = GetAlbedoBindless(MaterialData).Sample(GetMaterialSamplerBindless(MaterialData), TexCoords);
#else
    const float4 AlbedoSample = AlbedoTex.Sample(MaterialSampler, TexCoords);
#endif
    if (AlbedoSample.a < 0.5)
    {
        discard;
    }

    float3 SampledAlbedo = SRGBToLinear(AlbedoSample.rgb) * MaterialData.Albedo;
    
    const float3 WorldPosition = Input.WorldPosition;
    const float3 V             = normalize(CameraBuffer.PositionWS - WorldPosition);

#if BINDLESS_FORWARD_PASS
    float3 SampledNormal = GetNormalBindless(MaterialData).Sample(GetMaterialSamplerBindless(MaterialData), TexCoords).rgb;
#else
    float3 SampledNormal = NormalTex.Sample(MaterialSampler, TexCoords).rgb;
#endif
    SampledNormal = UnpackNormal(SampledNormal);
    SampledNormal = ApplyNormalMapAxis(SampledNormal, IsNormalMapPositiveY(MaterialData));

    float3 N = DecodeTangentNormal(SampledNormal, SurfaceNormal, Input.Tangent.xyz, TangentSign);

    // Sample packed materialparam texture (R=AO, G=Roughness, B=Metallic)
#if BINDLESS_FORWARD_PASS
    const float3 MaterialParams   = GetMaterialBindless(MaterialData).Sample(GetMaterialSamplerBindless(MaterialData), TexCoords);
#else
    const float3 MaterialParams   = MaterialMap.Sample(MaterialSampler, TexCoords);
#endif
    const float SampledAO        = MaterialParams.r * MaterialData.AO;
    const float SampledRoughness = MaterialParams.g * MaterialData.Roughness;
    const float SampledMetallic  = MaterialParams.b * MaterialData.Metallic;
    const float Roughness        = SampledRoughness;
    
    float3 F0 = 0.04;
    F0 = lerp(F0, SampledAlbedo, SampledMetallic);

    float  NDotV = max(dot(N, V), 0.0);
    float3 L0    = 0.0;
    
    // Pointlights
    for (int i = 0; i < 0; i++)
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
    
    for (int i = 0; i < 4; i++)
    {
        const FShadowPointLight Light       = ShadowCastingPointLights[i];
        const FPositionRadius   LightPosRad = ShadowCastingPointLightsPosRad[i];
     
        float ShadowFactor = PointLightShadowFactor(PointLightShadowMaps, float(i), ShadowMapSampler0, WorldPosition, N, Light, LightPosRad);
        if (ShadowFactor > 0.001)
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
    {
        const FDirectionalLight Light = DirLightBuffer;
        
        // TODO: FIX Shadows in forward
        
        //const float ShadowFactor = DirectionalLightShadowFactor(DirLightShadowMaps, ShadowMapSampler1, WorldPosition, N, Light, 0);
        //if (ShadowFactor > 0.001f)
        {
            float3 L = normalize(-Light.Direction);
            float3 H = normalize(L + V);
            
            float3 IncidentRadiance = Light.Color;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, SampledAlbedo, Roughness, SampledMetallic);
            
            L0 += IncidentRadiance;
        }
    }
    
    // Image Based Lightning
    float3 FinalColor = L0;

    {
        const float NDotV = max(dot(N, V), 0.0);
        
        float3 F  = FresnelSchlick_Roughness(F0, V, N, Roughness);
        float3 Ks = F;
        float3 Kd = 1.0 - Ks;

        float3 Irradiance      = IrradianceMap.SampleLevel(IrradianceSampler, N, 0.0).rgb;
        float3 Diffuse         = Irradiance * SampledAlbedo * Kd;
        float3 R               = reflect(-V, N);
        float3 PrefilteredMap  = SpecularIrradianceMap.SampleLevel(IrradianceSampler, R, Roughness * (7.0 - 1.0)).rgb;
        float2 BRDFIntegration = IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, Roughness), 0.0).rg;
        float3 Specular        = PrefilteredMap * (F * BRDFIntegration.x + BRDFIntegration.y);
        float3 Ambient         = (Diffuse + Specular) * SampledAO;

        FinalColor = Ambient + L0;
    }
    
    // Finalize
    float FinalLuminance = Luminance(FinalColor);
    return float4(FinalColor, FinalLuminance);
}
