#include "PBRHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Poisson.hlsli"
#include "Shadows/CascadeStructs.hlsli"
#include "Shadows/ShadowHelpers.hlsli"

#define NUM_THREADS 16
#define TOTAL_THREAD_COUNT (NUM_THREADS * NUM_THREADS)

#define BASE_OCCLUSION 0.1

// Can be defined from the application
#ifndef MAX_LIGHTS_PER_TILE
    #define MAX_LIGHTS_PER_TILE 1024
#endif

// Tile Occupancy Debug
#ifdef DRAW_TILE_DEBUG 
    #define DRAW_TILE_OCCUPANCY 1
#else
    #define DRAW_TILE_OCCUPANCY 0
#endif

// Cascade Debug
#ifdef DRAW_CASCADE_DEBUG
    #define DRAW_SHADOW_CASCADE 1
#else
    #define DRAW_SHADOW_CASCADE 0
#endif

// Enable Box-Projection for Light-Probes
#ifndef ENABLE_LIGHT_PROBE_BOX_PROJECTION
    #define ENABLE_LIGHT_PROBE_BOX_PROJECTION 1
#endif

// G-Buffer
Texture2D<float4> AlbedoTex       : register(t0);
Texture2D<float4> NormalBuffer    : register(t1);
Texture2D<float4> MaterialTex     : register(t2);
Texture2D<float>  DepthStencilTex : register(t3);

// Reflections
Texture2D<float4> DXRReflection : register(t4);

// Pre-integrated LUT
Texture2D<float2> IntegrationLUT : register(t5);

// SkyLight
TextureCube<float4> SkyLightDiffuseCubeMap  : register(t6);
TextureCube<float4> SkyLightSpecularCubeMap : register(t7);

// Light-Probe
TextureCube<float4> ProbeDiffuseCubeMap  : register(t8);
TextureCube<float4> ProbeSpecularCubeMap : register(t9);

// Shadow Cascade
Texture2D<float> DirectionalShadowMask : register(t10);

// Point Shadows
TextureCubeArray<float> PointLightShadowMaps : register(t11);

// SSAOBuffer
Texture2D<float> SSAOBuffer : register(t12);

// Shadow Cascade Data - (Debug data)
#if DRAW_SHADOW_CASCADE
Texture2D<uint> CascadeIndexBuffer : register(t13);
#endif

// Samplers
SamplerState LUTSampler         : register(s0);
SamplerState EnvironmentSampler : register(s1);
SamplerState GBufferSampler     : register(s2);

// Point-Lights
SamplerComparisonState ShadowMapSampler0 : register(s3);

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    int NumPointLights;
    int NumShadowCastingPointLights;
    int NumSkyLightMips;
    int NumLightProbes;

    // 16-32
    int ScreenWidth;
    int ScreenHeight;
    int bEnablePointLightShadows;
    int Padding0;
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

ConstantBuffer<FDirectionalLight> DirectionalLightBuffer : register(b5);
ConstantBuffer<FLightProbeInfo>   LightProbeInfoBuffer   : register(b6);

// Scene Output
RWTexture2D<float4> Output : register(u0);

// SpecularEnvironment

struct FSpecularEnvironmentInfo
{
    float3 ReflectionUVW;
    float  Roughness;
};

float3 SpecularEnvironment(TextureCube<float4> SpecularCubeMap, FSpecularEnvironmentInfo EnvironmentInfo)
{
    // Use a modified version of roughness when selecting miplevels
    float ModifiedRoughness = EnvironmentInfo.Roughness;
    ModifiedRoughness *= 1.7 - (0.7 * ModifiedRoughness);

    // Calculate the miplevel that we want to sample
    const float SpecularMipLevel = ModifiedRoughness * ((float)(Constants.NumSkyLightMips) - 1.0);
    
    // Sample and return specular cube-map
    return SpecularCubeMap.SampleLevel(EnvironmentSampler, EnvironmentInfo.ReflectionUVW, SpecularMipLevel).rgb;
}

float2 GetIntegrationConstants(float NDotV, float Roughness)
{
    return IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, Roughness), 0.0).rg;
}

// Diffuse Environment

struct FDiffuseEnvironmentInfo
{
    float3 NormalUVW;
};

float3 DiffuseEnvironment(TextureCube<float4> DiffuseCubeMap, FDiffuseEnvironmentInfo EnvironmentInfo)
{
    // Sample and return the diffuse cube-map
    return DiffuseCubeMap.SampleLevel(EnvironmentSampler, EnvironmentInfo.NormalUVW, 0.0).rgb;
}

// Box-Projection
struct FBoxProjectionInfo
{
    float3 ReflectionUVW;
    float3 PositionWS;
    float3 CubeMapPositionWS;
    float3 BoxMinWS;
    float3 BoxMaxWS;
    float  BoxProjection;
};

float3 BoxProjection(FBoxProjectionInfo BoxProjectionInfo)
{
#if ENABLE_LIGHT_PROBE_BOX_PROJECTION
    [[branch]]
    if (BoxProjectionInfo.BoxProjection > 0.0)
    {
        const float3 ReflectionUVW   = BoxProjectionInfo.ReflectionUVW;
        const float3 Position        = BoxProjectionInfo.PositionWS;        // viewer's position
        const float3 CubeMapPosition = BoxProjectionInfo.CubeMapPositionWS; // probe's world position

        // Compute the relative positions from the viewer.
        float3 RelativeMin = BoxProjectionInfo.BoxMinWS - Position;
        float3 RelativeMax = BoxProjectionInfo.BoxMaxWS - Position;

        float x = (ReflectionUVW.x > 0 ? RelativeMax.x : RelativeMin.x) / ReflectionUVW.x;
        float y = (ReflectionUVW.y > 0 ? RelativeMax.y : RelativeMin.y) / ReflectionUVW.y;
        float z = (ReflectionUVW.z > 0 ? RelativeMax.z : RelativeMin.z) / ReflectionUVW.z;

        float Scalar = min(min(x, y), z);
        
        // Return the new sampling direction.
        return ReflectionUVW * Scalar + (Position - CubeMapPosition);
    }
    else
#endif
    {
        return BoxProjectionInfo.ReflectionUVW;
    }
}

bool IsInsideAABB(float3 Position, float3 BoxMin, float3 BoxMax)
{
    return (Position.x >= BoxMin.x && Position.x <= BoxMax.x) && (Position.y >= BoxMin.y && Position.y <= BoxMax.y) && (Position.z >= BoxMin.z && Position.z <= BoxMax.z);
}

// Tiled Light Culling
groupshared uint GGroupMinZ;
groupshared uint GGroupMaxZ;
groupshared uint GGroupPointLightCounter;
groupshared uint GGroupPointLightIndices[MAX_LIGHTS_PER_TILE];
groupshared uint GGroupShadowPointLightCounter;
groupshared uint GGroupShadowPointLightIndices[MAX_LIGHTS_PER_TILE];

float GetNumTilesX()
{
    return DivideByMultiple(Constants.ScreenWidth, NUM_THREADS);
}

float GetNumTilesY()
{
    return DivideByMultiple(Constants.ScreenHeight, NUM_THREADS);
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint ThreadIndex = GroupThreadID.y * NUM_THREADS + GroupThreadID.x;
    if (ThreadIndex == 0)
    {
        GGroupMinZ = 0x7f7fffff;
        GGroupMaxZ = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint2 Pixel    = DispatchThreadID.xy;
    float Depth    = DepthStencilTex.Load(int3(Pixel, 0));
    float ViewPosZ = Depth_ProjToView(Depth, CameraBuffer.ProjectionInv);

    // TODO: If we change to reversed Z then we need to change from 1.0 to 0.0
    uint z = asuint(ViewPosZ);
    if (Depth < 1.0)
    {
        InterlockedMin(GGroupMinZ, z);
        InterlockedMax(GGroupMaxZ, z);
    }

    GroupMemoryBarrierWithGroupSync();

    float MinZ = asfloat(GGroupMinZ);
    float MaxZ = asfloat(GGroupMaxZ);

    float4 Frustum[4];

    {
        float pxm    = float(NUM_THREADS * GroupID.x);
        float pym    = float(NUM_THREADS * GroupID.y);
        float pxp    = float(NUM_THREADS * (GroupID.x + 1));
        float pyp    = float(NUM_THREADS * (GroupID.y + 1));
        float Width  = NUM_THREADS * GetNumTilesX();
        float Height = NUM_THREADS * GetNumTilesY();

        float3 CornerPoints[4];
        CornerPoints[0] = Float3_ProjToView(
            float3((pxm / Width) * 2.0 - 1.0, ((Height - pym) / Height) * 2.0 - 1.0, 1.0),
            CameraBuffer.ProjectionInv);

        CornerPoints[1] = Float3_ProjToView(
            float3((pxp / Width) * 2.0 - 1.0, ((Height - pym) / Height) * 2.0- 1.0, 1.0),
            CameraBuffer.ProjectionInv);

        CornerPoints[2] = Float3_ProjToView(
            float3((pxp / Width) * 2.0 - 1.0, ((Height - pyp) / Height) * 2.0 - 1.0, 1.0),
            CameraBuffer.ProjectionInv);

        CornerPoints[3] = Float3_ProjToView(
            float3((pxm / Width) * 2.0 - 1.0, ((Height - pyp) / Height) * 2.0 - 1.0, 1.0),
            CameraBuffer.ProjectionInv);

        for (uint i = 0; i < 4; i++)
        {
            Frustum[i] = CreatePlane(CornerPoints[i], CornerPoints[(i + 1) & 3]);
        }
    }

    if (ThreadIndex == 0)
    {
        GGroupPointLightCounter       = 0;
        GGroupShadowPointLightCounter = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    [loop]
    for (uint i = ThreadIndex; i < Constants.NumPointLights; i += TOTAL_THREAD_COUNT)
    {
        float3 Pos     = PointLightsPosRad[i].Position;
        float3 ViewPos = mul(float4(Pos, 1.0), CameraBuffer.View).xyz;
        float  Radius  = PointLightsPosRad[i].Radius;

        if ((GetSignedDistanceFromPlane(ViewPos, Frustum[0]) < Radius) && 
            (GetSignedDistanceFromPlane(ViewPos, Frustum[1]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[2]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[3]) < Radius) &&
            (-ViewPos.z + MinZ < Radius) && (ViewPos.z - MaxZ < Radius))
        {
            uint Index = 0;
            InterlockedAdd(GGroupPointLightCounter, 1, Index);
            GGroupPointLightIndices[Index] = i;
        }
    }

    // Cull point-light shadows
    [loop]
    for (uint j = ThreadIndex; j < Constants.NumShadowCastingPointLights; j += TOTAL_THREAD_COUNT)
    {
        float3 Pos     = ShadowCastingPointLightsPosRad[j].Position;
        float3 ViewPos = mul(float4(Pos, 1.0), CameraBuffer.View).xyz;
        float  Radius  = ShadowCastingPointLightsPosRad[j].Radius;

        if ((GetSignedDistanceFromPlane(ViewPos, Frustum[0]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[1]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[2]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[3]) < Radius) &&
            (-ViewPos.z + MinZ < Radius) && (ViewPos.z - MaxZ < Radius))
        {
            uint Index = 0;
            InterlockedAdd(GGroupShadowPointLightCounter, 1, Index);
            GGroupShadowPointLightIndices[Index] = j;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // Discard pixels not rendered to the GBuffer
    [[branch]]
    if (Depth == 1.0)
    {
        Output[Pixel] = 0.0;
        return;
    }

    const float2 PixelFloat   = saturate((float2(Pixel) + 0.5) / float2(Constants.ScreenWidth, Constants.ScreenHeight));
    const float3 ViewPosition = PositionFromDepth(Depth, PixelFloat, CameraBuffer.ProjectionInv);
    const float3 PositionWS   = mul(float4(ViewPosition, 1.0), CameraBuffer.ViewInv).xyz;

    const float3 GBufferNormal   = NormalBuffer.Load(int3(Pixel, 0)).rgb;
    const float3 GBufferAlbedo   = saturate(AlbedoTex.Load(int3(Pixel, 0)).rgb);
    const float3 GBufferMaterial = MaterialTex.Load(int3(Pixel, 0)).rgb;

    // Sample with a sampler since the texture is not necessarilly the same size as the screen
    const float ScreenSpaceAO = SSAOBuffer.SampleLevel(GBufferSampler, PixelFloat, 0).r;
    
    const float3 NormalWS = UnpackNormal(GBufferNormal);
    const float3 ViewWS   = normalize(CameraBuffer.PositionWS - PositionWS);

    const float GBufferRoughness = saturate(GBufferMaterial.r);
    const float GBufferMetallic  = saturate(GBufferMaterial.g);
    const float GBufferAO        = saturate(BASE_OCCLUSION + (GBufferMaterial.b * ScreenSpaceAO));
    
    float3 F0 = 0.04;
    F0 = lerp(F0, GBufferAlbedo, GBufferMetallic);

    float3 L0 = 0.0;

    // Pointlights
    [loop]
    for (uint i = 0; i < GGroupPointLightCounter; ++i)
    {
        const int Index = GGroupPointLightIndices[i];

        const FPointLight     Light       = PointLights[Index];
        const FPositionRadius LightPosRad = PointLightsPosRad[Index];

        float3 L = LightPosRad.Position - PositionWS;
        float  DistanceSqrd = dot(L, L);
        float  Attenuation  = 1.0 / max(DistanceSqrd, 0.01 * 0.01);
        L = normalize(L);

        float3 IncidentRadiance = Light.Color * Attenuation;
        IncidentRadiance = DirectRadiance(F0, NormalWS, ViewWS, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);
            
        L0 += IncidentRadiance;
    }

    // Point-light shadows
    [loop]
    for (uint i = 0; i < GGroupShadowPointLightCounter; i++)
    {
        int Index = GGroupShadowPointLightIndices[i];
        const FShadowPointLight Light       = ShadowCastingPointLights[Index];
        const FPositionRadius   LightPosRad = ShadowCastingPointLightsPosRad[Index];

        float ShadowFactor;
        
        [branch]
        if (Constants.bEnablePointLightShadows)
        {
            ShadowFactor = PointLightShadowFactor(PointLightShadowMaps, float(Index), ShadowMapSampler0, PositionWS, NormalWS, Light, LightPosRad);
        }
        else
        {
            ShadowFactor = 1.0;
        }

        [branch]
        if (ShadowFactor > 0.001)
        {
            float3 L = LightPosRad.Position - PositionWS;
            float  DistanceSqrd = dot(L, L);
            float  Attenuation  = 1.0 / max(DistanceSqrd, 0.01 * 0.01);
            L = normalize(L);

            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(F0, NormalWS, ViewWS, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);

            L0 += IncidentRadiance * ShadowFactor;
        }
    }

    // DirectionalLights
    float ShadowMask = DirectionalShadowMask.Load(int3(Pixel, 0));

    {
        const FDirectionalLight Light = DirectionalLightBuffer;
        float3 L = normalize(-Light.Direction);
        
        [branch]
        if (ShadowMask > 0.0)
        {
            float3 IncidentRadiance = Light.Color;
            IncidentRadiance = DirectRadiance(F0, NormalWS, ViewWS, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);      
            L0 += IncidentRadiance * ShadowMask;
        }
    }

    // Modify shadow-mask when sampling environment
    ShadowMask = max(0.7, ShadowMask);

    // Image Based Lightning
    float3 FinalColor = L0;

    {
        float  NDotV      = max(dot(NormalWS, ViewWS), 0.0);
        float3 Reflection = reflect(-ViewWS, NormalWS);
        
        float3 F  = FresnelSchlick_Roughness(F0, ViewWS, NormalWS, GBufferRoughness);
        float3 Ks = F;
        float3 Kd = 1.0 - Ks;

        // Sample cube-maps
        FDiffuseEnvironmentInfo DiffuseEnvironmentInfo;
        DiffuseEnvironmentInfo.NormalUVW = NormalWS;

        FSpecularEnvironmentInfo SpecularEnvironmentInfo;
        SpecularEnvironmentInfo.Roughness = GBufferRoughness;

        float3 DiffuseSample;
        float3 SpecularSample;
        
        [[branch]]
        if (Constants.NumLightProbes > 0)
        {
            [[branch]]
            if (IsInsideAABB(PositionWS, LightProbeInfoBuffer.BoxMinWS, LightProbeInfoBuffer.BoxMaxWS))
            {
                FBoxProjectionInfo BoxProjectionInfo;
                BoxProjectionInfo.ReflectionUVW     = normalize(Reflection);
                BoxProjectionInfo.PositionWS        = PositionWS;
                BoxProjectionInfo.CubeMapPositionWS = LightProbeInfoBuffer.BoxOriginWS;
                BoxProjectionInfo.BoxMinWS          = LightProbeInfoBuffer.BoxMinWS;
                BoxProjectionInfo.BoxMaxWS          = LightProbeInfoBuffer.BoxMaxWS;
                BoxProjectionInfo.BoxProjection     = LightProbeInfoBuffer.BoxProjection;
                
                SpecularEnvironmentInfo.ReflectionUVW = BoxProjection(BoxProjectionInfo);

                SpecularSample = SpecularEnvironment(ProbeSpecularCubeMap, SpecularEnvironmentInfo);
                DiffuseSample  = DiffuseEnvironment(ProbeDiffuseCubeMap, DiffuseEnvironmentInfo);
            }
            else
            {
                SpecularEnvironmentInfo.ReflectionUVW = Reflection;

                // Apply shadow-mask so that environment is not too visible in the shadowed areas
                SpecularSample = SpecularEnvironment(SkyLightSpecularCubeMap, SpecularEnvironmentInfo) * ShadowMask;
                DiffuseSample  = DiffuseEnvironment(SkyLightDiffuseCubeMap, DiffuseEnvironmentInfo) * ShadowMask;
            }
        }
        else
        {
            SpecularEnvironmentInfo.ReflectionUVW = Reflection;

            // Apply shadow-mask so that environment is not too visible in the shadowed areas
            SpecularSample = SpecularEnvironment(SkyLightSpecularCubeMap, SpecularEnvironmentInfo)* ShadowMask;
            DiffuseSample  = DiffuseEnvironment(SkyLightDiffuseCubeMap, DiffuseEnvironmentInfo) * ShadowMask;
        }

        // Perform calculations
        float2 BRDFIntegration = GetIntegrationConstants(NDotV, GBufferRoughness);
        
        float3 Specular = SpecularSample * (Ks * BRDFIntegration.x + BRDFIntegration.y);
        float3 Diffuse  = DiffuseSample * GBufferAlbedo * Kd;
        float3 Ambient  = (Diffuse + Specular) * GBufferAO;
        FinalColor = Ambient + L0;
        // FinalColor = SpecularSample;
    }

#if DRAW_TILE_OCCUPANCY
    const uint TotalLightCount = GGroupPointLightCounter + GGroupShadowPointLightCounter;
    
    float4 Tint = 1.0;
    
    [[branch]]
    if (TotalLightCount > 0)
    {
        if (TotalLightCount < 8)
        {
            float Color = float(TotalLightCount) / 8.0;
            Tint = float4(0.0, Color, 0.0, 1.0);
        }
        else if (TotalLightCount < 16)
        {
            float Color = float(TotalLightCount) / 16.0;
            Tint = float4(0.0, Color, Color, 1.0);
        }
        else if (TotalLightCount < 32)
        {
            float Color = float(TotalLightCount) / 32.0;
            Tint = float4(0.0, 0.0, Color, 1.0);
        }
        else if (TotalLightCount < 64)
        {
            float Color = float(TotalLightCount) / 64.0;
            Tint = float4(Color, Color, 0.0, 1.0);
        }
        else
        {
            float Color = float(TotalLightCount) / float(Constants.NumPointLights + Constants.NumShadowCastingPointLights);
            Tint = float4(Color, 0.0, 0.0, 1.0);
        }
    }
    
    FinalColor = FinalColor * Tint.rgb;

#elif DRAW_SHADOW_CASCADE
    const uint CascadeIndex = CascadeIndexBuffer[Pixel];

    float4 Tint = 1.0;
    if (CascadeIndex == 0)
    {
        Tint = float4(1.0, 0.0, 0.0, 1.0);
    }
    else if (CascadeIndex == 1)
    {
        Tint = float4(0.0, 1.0, 0.0, 1.0);
    }
    else if (CascadeIndex == 2)
    {
        Tint = float4(0.0, 0.0, 1.0, 1.0);
    }
    else if (CascadeIndex == 3)
    {
        Tint = float4(1.0, 1.0, 0.0, 1.0);
    }

    FinalColor = FinalColor * Tint.rgb;
#endif
    
    // Finalize
    float FinalLuminance = Luminance(FinalColor);
    Output[Pixel] = float4(FinalColor, FinalLuminance);
}