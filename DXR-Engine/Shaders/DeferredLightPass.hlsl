#include "PBRHelpers.hlsli"
#include "ShadowHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"

#define THREAD_COUNT 16

Texture2D<float4>   AlbedoTex               : register(t0, space0);
Texture2D<float4>   NormalTex               : register(t1, space0);
Texture2D<float4>   MaterialTex             : register(t2, space0);
Texture2D<float>    DepthStencilTex         : register(t3, space0);
Texture2D<float4>   DXRReflection           : register(t4, space0);
TextureCube<float4> IrradianceMap           : register(t5, space0);
TextureCube<float4> SpecularIrradianceMap   : register(t6, space0);
Texture2D<float4>   IntegrationLUT          : register(t7, space0);
Texture2D<float>    DirLightShadowMaps      : register(t8, space0);
TextureCube<float>  PointLightShadowMaps    : register(t9, space0);
Texture2D<float3>   SSAO                    : register(t10, space0);

SamplerState GBufferSampler     : register(s0, space0);
SamplerState LUTSampler         : register(s1, space0);
SamplerState IrradianceSampler  : register(s2, space0);

SamplerComparisonState ShadowMapSampler0    : register(s3, space0);
SamplerState ShadowMapSampler1              : register(s4, space0);

cbuffer Constants : register(b0, space0)
{
    uint NumSkyLightMips;
    uint ScreenWidth;
    uint ScreenHeight;
};

ConstantBuffer<Camera>              CameraBuffer        : register(b1, space0);
ConstantBuffer<PointLight>          PointLightBuffer    : register(b2, space0);
ConstantBuffer<DirectionalLight>    DirLightBuffer      : register(b3, space0);

RWTexture2D<float4> Output : register(u0, space0);

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
    const int2 TexCoord = int2(Input.DispatchThreadID.xy);
    const float Depth   = DepthStencilTex.Load(int3(TexCoord, 0));
    if (Depth >= 1.0f)
    {
        Output[TexCoord] = float4(Float3(0.0f), 1.0f);
        return;
    }

    const float2 TexCoordFloat      = float2(TexCoord) / float2(ScreenWidth, ScreenHeight);
    const float3 WorldPosition      = PositionFromDepth(Depth, TexCoordFloat, CameraBuffer.ViewProjectionInverse);
    const float3 GBufferAlbedo      = AlbedoTex.Load(int3(TexCoord, 0)).rgb;
    const float3 GBufferMaterial    = MaterialTex.Load(int3(TexCoord, 0)).rgb;
    const float3 GBufferNormal      = NormalTex.Load(int3(TexCoord, 0)).rgb;
    const float ScreenSpaceAO       = SSAO.Load(int3(TexCoord, 0)).r;
    
    const float3 N = UnpackNormal(GBufferNormal);
    const float3 V = normalize(CameraBuffer.Position - WorldPosition);
    const float GBufferRoughness    = GBufferMaterial.r;
    const float GBufferMetallic     = GBufferMaterial.g;
    const float GBufferAO           = GBufferMaterial.b * ScreenSpaceAO;
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, GBufferAlbedo, GBufferMetallic);
    
    float3 L0 = Float3(0.0f);
    
    // Pointlights
    {
        const PointLight Light = PointLightBuffer;
        const float ShadowFactor = PointLightShadowFactor(
            PointLightShadowMaps,
            ShadowMapSampler0,
            WorldPosition,
            N,
            Light);
        
        if (ShadowFactor > 0.1f)
        {
            float3 L = normalize(Light.Position - WorldPosition);
            float3 H = normalize(L + V);
            float Distance      = length(Light.Position - WorldPosition);
            float Attenuation   = 1.0f / (Distance * Distance);
            
            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(
                F0, N, V, L,
                IncidentRadiance,
                GBufferAlbedo,
                GBufferRoughness,
                GBufferMetallic);
            
            L0 += IncidentRadiance * ShadowFactor;
        }
    }
    
    // DirectionalLights
    {
        const DirectionalLight Light = DirLightBuffer;
        const float ShadowFactor = DirectionalLightShadowFactor(
            DirLightShadowMaps,
            ShadowMapSampler0,
            WorldPosition,
            N,
            Light);
        
        if (ShadowFactor > 0.1f)
        {
            float3 L = normalize(-Light.Direction);
            float3 H = normalize(L + V);            
            
            float3 IncidentRadiance = Light.Color;
            IncidentRadiance = DirectRadiance(
                F0, N, V, L,
                IncidentRadiance,
                GBufferAlbedo,
                GBufferRoughness,
                GBufferMetallic);
            
            L0 += IncidentRadiance * ShadowFactor;
        }
    }
    
    // Image Based Lightning
    float3 FinalColor = L0;
    {
        const float NDotV = max(dot(N, V), 0.0f);
        
        float3 F    = FresnelSchlick_Roughness(F0, V, N, GBufferRoughness);
        float3 Ks   = F;
        float3 Kd   = Float3(1.0f) - Ks;
        float3 Irradiance   = IrradianceMap.SampleLevel(IrradianceSampler, N, 0.0f).rgb;
        float3 Diffuse      = Irradiance * GBufferAlbedo * Kd;

        float3 R = reflect(-V, N);
        float3 PrefilteredMap   = SpecularIrradianceMap.SampleLevel(IrradianceSampler, R, GBufferRoughness * (NumSkyLightMips - 1.0f)).rgb;
        float2 BRDFIntegration  = IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, GBufferRoughness), 0.0f).rg;
        float3 Specular         = PrefilteredMap * (F * BRDFIntegration.x + BRDFIntegration.y);

        float3 Ambient  = (Diffuse + Specular) * GBufferAO;
        FinalColor      = Ambient + L0;
    }
    
    // Finalize
    FinalColor = ApplyGammaCorrectionAndTonemapping(FinalColor);
    float Luminance     = Luma(FinalColor);
    Output[TexCoord]    = float4(FinalColor, Luminance);
}