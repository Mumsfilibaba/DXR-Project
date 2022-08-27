#include "PBRHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "ShadowHelpers.hlsli"

// Per Frame Buffers

// TODO: Fix this
//cbuffer Constants : register(b0, space0)
//{
//    int NumPointLights;
//    int NumSkyLightMips;
//};

ConstantBuffer<FCamera> CameraBuffer : register(b0, space0);

cbuffer PointLightsBuffer : register(b1, space0)
{
    FPointLight PointLights[32];
}

cbuffer PointLightsPosRadBuffer : register(b2, space0)
{
    FPositionRadius PointLightsPosRad[32];
}

cbuffer ShadowCastingPointLightsBuffer : register(b3, space0)
{
    FShadowPointLight ShadowCastingPointLights[8];
}

cbuffer ShadowCastingPointLightsPosRadBuffer : register(b4, space0)
{
    FPositionRadius ShadowCastingPointLightsPosRad[8];
}

ConstantBuffer<FDirectionalLight> DirLightBuffer : register(b5, space0);

// Per Object Buffers
ConstantBuffer<FTransform> TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);
ConstantBuffer<FMaterial> MaterialBuffer : register(b6);

// Per Frame Samplers
SamplerState MaterialSampler   : register(s0);
SamplerState LUTSampler        : register(s1);
SamplerState IrradianceSampler : register(s2);

SamplerComparisonState ShadowMapSampler0 : register(s3);
SamplerComparisonState ShadowMapSampler1 : register(s4);

// Per Frame Textures
TextureCube<float4>     IrradianceMap         : register(t0);
TextureCube<float4>     SpecularIrradianceMap : register(t1);
Texture2D<float4>       IntegrationLUT        : register(t2);
Texture2D<float>        DirLightShadowMaps    : register(t3);
TextureCubeArray<float> PointLightShadowMaps  : register(t4);

// Per Object Textures
Texture2D<float4> AlbedoTex   : register(t5);
Texture2D<float4> NormalTex   : register(t6);
Texture2D<float> RoughnessTex : register(t7);
Texture2D<float> HeightMap    : register(t8);
Texture2D<float> MetallicTex  : register(t9);
Texture2D<float> AOTex        : register(t10);
Texture2D<float> AlphaTex     : register(t11);

struct VSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float3 WorldPosition   : POSITION0;
    float3 Normal          : NORMAL0;
    float3 Tangent         : TANGENT0;
    float3 Bitangent       : BITANGENT0;
    float2 TexCoord        : TEXCOORD0;
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
    float4 Position        : SV_Position;
};

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;
    
    float3 Normal = normalize(mul(float4(Input.Normal, 0.0f), TransformBuffer.Transform).xyz);
    Output.Normal = Normal;
    
    float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), TransformBuffer.Transform).xyz);
    Tangent        = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Output.Tangent = Tangent;
    
    float3 Bitangent = normalize(cross(Output.Tangent, Output.Normal));
    Output.Bitangent = Bitangent;

    Output.TexCoord = Input.TexCoord;

    float4 WorldPosition = mul(float4(Input.Position, 1.0f), TransformBuffer.Transform);
    Output.Position      = mul(WorldPosition, CameraBuffer.ViewProjection);
    Output.WorldPosition = WorldPosition.xyz;

    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    TBN          = transpose(TBN);
    
    Output.TangentViewPos  = mul(CameraBuffer.Position, TBN);
    Output.TangentPosition = mul(WorldPosition.xyz, TBN);

    return Output;
}

struct PSInput
{
    float3 WorldPosition   : POSITION0;
    float3 Normal          : NORMAL0;
    float3 Tangent         : TANGENT0;
    float3 Bitangent       : BITANGENT0;
    float2 TexCoord        : TEXCOORD0;
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
    bool   bIsFrontFace    : SV_IsFrontFace;
};

static const float HEIGHT_SCALE = 0.03f;

float SampleHeightMap(float2 TexCoords)
{
    return 1.0f - HeightMap.Sample(MaterialSampler, TexCoords).r;
}

float2 ParallaxMapping(float2 TexCoords, float3 ViewDir)
{
    const float MinLayers = 32;
    const float MaxLayers = 64;

    float NumLayers  = lerp(MaxLayers, MinLayers, abs(dot(float3(0.0f, 0.0f, 1.0f), ViewDir)));
    float LayerDepth = 1.0f / NumLayers;
    
    float2 P = ViewDir.xy / ViewDir.z * HEIGHT_SCALE;
    float2 DeltaTexCoords = P / NumLayers;

    float2 CurrentTexCoords     = TexCoords;
    float  CurrentDepthMapValue = SampleHeightMap(CurrentTexCoords);
    
    float CurrentLayerDepth = 0.0f;
    while (CurrentLayerDepth < CurrentDepthMapValue)
    {
        CurrentTexCoords     -= DeltaTexCoords;
        CurrentDepthMapValue = SampleHeightMap(CurrentTexCoords);
        CurrentLayerDepth    += LayerDepth;
    }

    float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

    float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    float BeforeDepth = SampleHeightMap(PrevTexCoords) - CurrentLayerDepth + LayerDepth;

    float  Weight         = AfterDepth / (AfterDepth - BeforeDepth);
    float2 FinalTexCoords = PrevTexCoords * Weight + CurrentTexCoords * (1.0f - Weight);

    return FinalTexCoords;
}

float4 PSMain(PSInput Input) : SV_Target0
{
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;
    
    if (MaterialBuffer.EnableHeight != 0)
    {
        float3 ViewDir = normalize(Input.TangentViewPos - Input.TangentPosition);
        TexCoords      = ParallaxMapping(TexCoords, ViewDir);
        if (TexCoords.x > 1.0f || TexCoords.y > 1.0f || TexCoords.x < 0.0f || TexCoords.y < 0.0f)
        {
            discard;
        }
    }

    float3 SampledAlbedo = ApplyGamma(AlbedoTex.Sample(MaterialSampler, TexCoords).rgb) * MaterialBuffer.Albedo;
    
    const float3 WorldPosition = Input.WorldPosition;
    const float3 V             = normalize(CameraBuffer.Position - WorldPosition);
    float3 N = normalize(Input.Normal);
    if (!Input.bIsFrontFace)
    {
        N = -N;
    }
    
    float3 SampledNormal = NormalTex.Sample(MaterialSampler, TexCoords).rgb;
    SampledNormal        = UnpackNormal(SampledNormal);
    
    float3 Tangent   = normalize(Input.Tangent);
    float3 Bitangent = normalize(Input.Bitangent);
    float3 Normal    = normalize(N);
    N = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);

    const float SampledAO        = AOTex.Sample(MaterialSampler, TexCoords) * MaterialBuffer.AO;
    const float SampledMetallic  = MetallicTex.Sample(MaterialSampler, TexCoords) * MaterialBuffer.Metallic;
    const float SampledRoughness = RoughnessTex.Sample(MaterialSampler, TexCoords) * MaterialBuffer.Roughness;
    const float	SampledAlpha     = AlphaTex.Sample(MaterialSampler, TexCoords);
    const float Roughness        = SampledRoughness;
    if (SampledAlpha < 0.5f)
    {
        discard;
    }
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, SampledAlbedo, SampledMetallic);

    float NDotV = max(dot(N, V), 0.0f);
    float3 L0 = Float3(0.0f);
    
    // Pointlights
    for (int i = 0; i < 0; i++)
    {
        const FPointLight     Light       = PointLights[i];
        const FPositionRadius LightPosRad = PointLightsPosRad[i];

        float3 L = LightPosRad.Position - WorldPosition;
        float DistanceSqrd = dot(L, L);
        float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
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
        if (ShadowFactor > 0.001f)
        {
            float3 L = LightPosRad.Position - WorldPosition;
            float DistanceSqrd = dot(L, L);
            float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
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
        const float NDotV = max(dot(N, V), 0.0f);
        
        float3 F  = FresnelSchlick_Roughness(F0, V, N, Roughness);
        float3 Ks = F;
        float3 Kd = Float3(1.0f) - Ks;
        float3 Irradiance = IrradianceMap.SampleLevel(IrradianceSampler, N, 0.0f).rgb;
        float3 Diffuse    = Irradiance * SampledAlbedo * Kd;

        float3 R = reflect(-V, N);
        float3 PrefilteredMap  = SpecularIrradianceMap.SampleLevel(IrradianceSampler, R, Roughness * (7.0f - 1.0f)).rgb;
        float2 BRDFIntegration = IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, Roughness), 0.0f).rg;
        float3 Specular        = PrefilteredMap * (F * BRDFIntegration.x + BRDFIntegration.y);

        float3 Ambient = (Diffuse + Specular) * SampledAO;
        FinalColor = Ambient + L0;
    }
    
    // Finalize
    float FinalLuminance = Luminance(FinalColor);
    FinalColor = ApplyGammaCorrectionAndTonemapping(FinalColor);
    return float4(FinalColor, FinalLuminance);
}