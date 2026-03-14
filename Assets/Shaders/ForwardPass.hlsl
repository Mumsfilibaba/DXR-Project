#include "PBRHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "Shadows/CascadeStructs.hlsli"
#include "Shadows/ShadowHelpers.hlsli"
#include "ParallaxMapping.hlsli"

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

ConstantBuffer<FDirectionalLight> DirLightBuffer : register(b5);

// Per Object Buffers
ConstantBuffer<FTransform> TransformBuffer : register(b1);
ConstantBuffer<FMaterial>  MaterialBuffer  : register(b6);

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
Texture2D<float4> AlbedoTex    : register(t5);
Texture2D<float4> NormalTex    : register(t6);
Texture2D<float3> MaterialMap  : register(t7);
Texture2D<float>  HeightMap    : register(t8);

struct FVSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct FVSOutput
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

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;
    
    float3 Normal = normalize(TransformDirectionWS(TransformBuffer, Input.Normal));
    Output.Normal = Normal;
    
    float3 Tangent = normalize(TransformDirectionWS(TransformBuffer, Input.Tangent));
    Tangent        = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Output.Tangent = Tangent;
    
    float3 Bitangent = normalize(cross(Output.Tangent, Output.Normal));
    Output.Bitangent = Bitangent;

    Output.TexCoord = Input.TexCoord;

    const float3 WorldPosition3 = TransformPositionWS(TransformBuffer, Input.Position);
    Output.Position      = mul(float4(WorldPosition3, 1.0), CameraBuffer.ViewProjection);
    Output.WorldPosition = WorldPosition3;

    float3x3 TangentSpace = float3x3(Tangent, Bitangent, Normal);
    TangentSpace          = transpose(TangentSpace);
    
    Output.TangentViewPos  = mul(CameraBuffer.PositionWS, TangentSpace);
    Output.TangentPosition = mul(WorldPosition3, TangentSpace);

    return Output;
}

struct FPSInput
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

float4 PSMain(FPSInput Input) : SV_Target0
{
    float2 TexCoords = Input.TexCoord;

#if 0 
    if (MaterialBuffer.EnableHeight != 0)
    {
        const float2 TexCoordsDx = ddx(TexCoords);
        const float2 TexCoordsDy = ddy(TexCoords);

        float3 ViewDir = normalize(Input.TangentViewPos - Input.TangentPosition);

        uint bParallaxDiscard = 0;
        TexCoords = ParallaxMapUV(HeightMap, MaterialSampler, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialBuffer.ParallaxHeightScale, MaterialBuffer.ParallaxMinLayers, MaterialBuffer.ParallaxMaxLayers, bParallaxDiscard);
        if (bParallaxDiscard != 0)
        {
            discard;
        }
    }
#endif

    const float4 AlbedoSample = AlbedoTex.Sample(MaterialSampler, TexCoords);
    if (AlbedoSample.a < 0.5)
    {
        discard;
    }

    float3 SampledAlbedo = SRGBToLinear(AlbedoSample.rgb) * MaterialBuffer.Albedo;
    
    const float3 WorldPosition = Input.WorldPosition;
    const float3 V             = normalize(CameraBuffer.PositionWS - WorldPosition);

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

    // Sample packed materialparam texture (R=AO, G=Roughness, B=Metallic)
    const float3 MaterialParams   = MaterialMap.Sample(MaterialSampler, TexCoords);
    const float  SampledAO        = MaterialParams.r * MaterialBuffer.AO;
    const float  SampledRoughness = MaterialParams.g * MaterialBuffer.Roughness;
    const float  SampledMetallic  = MaterialParams.b * MaterialBuffer.Metallic;
    const float  Roughness        = SampledRoughness;
    
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
            float3 L = LightPosRad.Position - WorldPosition;
            float DistanceSqrd = dot(L, L);
            float Attenuation  = 1.0 / max(DistanceSqrd, 0.01 * 0.01);
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

        float3 Irradiance = IrradianceMap.SampleLevel(IrradianceSampler, N, 0.0).rgb;
        float3 Diffuse    = Irradiance * SampledAlbedo * Kd;

        float3 R               = reflect(-V, N);
        float3 PrefilteredMap  = SpecularIrradianceMap.SampleLevel(IrradianceSampler, R, Roughness * (7.0 - 1.0)).rgb;
        float2 BRDFIntegration = IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, Roughness), 0.0).rg;
        float3 Specular        = PrefilteredMap * (F * BRDFIntegration.x + BRDFIntegration.y);

        float3 Ambient = (Diffuse + Specular) * SampledAO;
        FinalColor = Ambient + L0;
    }
    
    // Finalize
    float FinalLuminance = Luminance(FinalColor);
    return float4(FinalColor, FinalLuminance);
}
