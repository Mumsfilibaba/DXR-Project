#include "PBRHelpers.hlsli"
#include "ShadowHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"

#define THREAD_COUNT        16
#define TOTAL_THREAD_COUNT  (THREAD_COUNT * THREAD_COUNT)
#define MAX_LIGHTS_PER_TILE 256

#define DRAW_TILE_OCCUPANCY 0

Texture2D<float4>       AlbedoTex               : register(t0, space0);
Texture2D<float4>       NormalTex               : register(t1, space0);
Texture2D<float4>       MaterialTex             : register(t2, space0);
Texture2D<float>        DepthStencilTex         : register(t3, space0);
Texture2D<float4>       DXRReflection           : register(t4, space0);
TextureCube<float4>     IrradianceMap           : register(t5, space0);
TextureCube<float4>     SpecularIrradianceMap   : register(t6, space0);
Texture2D<float4>       IntegrationLUT          : register(t7, space0);
Texture2D<float>        DirLightShadowMaps      : register(t8, space0);
TextureCubeArray<float> PointLightShadowMaps    : register(t9, space0);
Texture2D<float3>       SSAO                    : register(t10, space0);

SamplerState GBufferSampler     : register(s0, space0);
SamplerState LUTSampler         : register(s1, space0);
SamplerState IrradianceSampler  : register(s2, space0);

SamplerComparisonState ShadowMapSampler0    : register(s3, space0);
SamplerState ShadowMapSampler1              : register(s4, space0);

cbuffer Constants : register(b0, space0)
{
    int NumPointLights;
    int NumSkyLightMips;
    int ScreenWidth;
    int ScreenHeight;
};

ConstantBuffer<Camera> CameraBuffer : register(b1, space0);

cbuffer PointLightsBuffer : register(b2, space0)
{
    PointLight PointLights[8];
}

ConstantBuffer<DirectionalLight> DirLightBuffer : register(b3, space0);

RWTexture2D<float4> Output : register(u0, space0);

// Tiled Light Culling
groupshared uint GroupMinZ;
groupshared uint GroupMaxZ;
groupshared uint GroupLightCounter;
groupshared uint GroupLightIndices[MAX_LIGHTS_PER_TILE];

float GetNumTilesX()
{
    return DivideByMultiple(ScreenWidth, THREAD_COUNT);
}

float GetNumTilesY()
{
    return DivideByMultiple(ScreenHeight, THREAD_COUNT);
}

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
    uint ThreadIndex = Input.GroupThreadID.y * THREAD_COUNT + Input.GroupThreadID.x;
    if (ThreadIndex == 0)
    {
        GroupMinZ = 0x7f7fffff;
        GroupMaxZ = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();

    uint2 TexCoord  = Input.DispatchThreadID.xy;
    float Depth     = DepthStencilTex.Load(int3(TexCoord, 0));
    float ViewPosZ  = Depth_ProjToView(Depth, CameraBuffer.ProjectionInverse);
    
    // TODO: If we change to reversed Z then we need to change from 1.0 to 0.0
    
    uint z = asuint(ViewPosZ);
    if (Depth < 1.0f)
    {
        InterlockedMin(GroupMinZ, z);
        InterlockedMax(GroupMaxZ, z);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    float MinZ = asfloat(GroupMinZ);
    float MaxZ = asfloat(GroupMaxZ);
    
    float4 Frustum[4];
    {
        float pxm    = float(THREAD_COUNT * Input.GroupID.x);
        float pym    = float(THREAD_COUNT * Input.GroupID.y);
        float pxp    = float(THREAD_COUNT * (Input.GroupID.x + 1));
        float pyp    = float(THREAD_COUNT * (Input.GroupID.y + 1));
        float Width  = THREAD_COUNT * GetNumTilesX();
        float Height = THREAD_COUNT * GetNumTilesY();
        
        float3 CornerPoints[4];
        CornerPoints[0] = Float3_ProjToView(
            float3((pxm / Width) * 2.0f - 1.0f, ((Height - pym) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);
        
        CornerPoints[1] = Float3_ProjToView(
            float3((pxp / Width) * 2.0f - 1.0f, ((Height - pym) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);
        
        CornerPoints[2] = Float3_ProjToView(
            float3((pxp / Width) * 2.0f - 1.0f, ((Height - pyp) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);
        
        CornerPoints[3] = Float3_ProjToView(
            float3((pxm / Width) * 2.0f - 1.0f, ((Height - pyp) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);

        for (uint i = 0; i < 4; i++)
        {
            Frustum[i] = CreatePlane(CornerPoints[i], CornerPoints[(i + 1) & 3]);
        }
    }
    
    if (ThreadIndex == 0)
    {
        GroupLightCounter = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    for (uint i = ThreadIndex; i < NumPointLights; i += TOTAL_THREAD_COUNT)
    {
        float3 Pos     = PointLights[i].Position;
        float3 ViewPos = mul(float4(Pos, 1.0f), CameraBuffer.View);
        float Radius   = PointLights[i].Radius;

        if ((GetSignedDistanceFromPlane(ViewPos, Frustum[0]) < Radius) && 
            (GetSignedDistanceFromPlane(ViewPos, Frustum[1]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[2]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[3]) < Radius) &&
            (-ViewPos.z + MinZ < Radius) && (ViewPos.z - MaxZ < Radius))
        {
            uint Index = 0;
            InterlockedAdd(GroupLightCounter, 1, Index);
            GroupLightIndices[Index] = i;
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    const float2 TexCoordFloat   = float2(TexCoord) / float2(ScreenWidth, ScreenHeight);
    const float3 WorldPosition   = PositionFromDepth(Depth, TexCoordFloat, CameraBuffer.ViewProjectionInverse);
    const float3 GBufferAlbedo   = AlbedoTex.Load(int3(TexCoord, 0)).rgb;
    const float3 GBufferMaterial = MaterialTex.Load(int3(TexCoord, 0)).rgb;
    const float3 GBufferNormal   = NormalTex.Load(int3(TexCoord, 0)).rgb;
    const float ScreenSpaceAO    = SSAO.Load(int3(TexCoord, 0)).r;
    
    const float3 N = UnpackNormal(GBufferNormal);
    const float3 V = normalize(CameraBuffer.Position - WorldPosition);
    const float GBufferRoughness = GBufferMaterial.r;
    const float GBufferMetallic  = GBufferMaterial.g;
    const float GBufferAO        = GBufferMaterial.b * ScreenSpaceAO;
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, GBufferAlbedo, GBufferMetallic);
    
    float3 L0 = Float3(0.0f);
    
    // Pointlights
    for (int i = 0; i < GroupLightCounter; i++)
    {
        int Index = GroupLightIndices[i];
        const PointLight Light = PointLights[Index];
        float ShadowFactor = PointLightShadowFactor(
            PointLightShadowMaps, float(Index),
            ShadowMapSampler0,
            WorldPosition,
            N,
            Light);
        
        if (ShadowFactor > 0.001f)
        {
            float3 L = Light.Position - WorldPosition;
            float DistanceSqrd  = dot(L, L);
            float Attenuation   = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
            L = normalize(L);
            
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
        
        if (ShadowFactor > 0.001f)
        {
            float3 L = normalize(-Light.Direction);
            
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
    
    float4 Tint = Float4(1.0f);
#if DRAW_TILE_OCCUPANCY
    if (GroupLightCounter == 0)
    {
        Tint = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else if (GroupLightCounter == 1)
    {
        Tint = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (GroupLightCounter == 2)
    {
        Tint = float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else if (GroupLightCounter == 3)
    {
        Tint = float4(0.0f, 0.0f, 1.0f, 1.0f);
    }
    else if (GroupLightCounter == 4)
    {
        Tint = float4(1.0f, 1.0f, 0.0f, 1.0f);
    }
#endif
    
    // Finalize
    FinalColor = ApplyGammaCorrectionAndTonemapping(FinalColor * Tint.rgb);
    float Luminance  = Luma(FinalColor);
    Output[TexCoord] = float4(FinalColor, Luminance);
}