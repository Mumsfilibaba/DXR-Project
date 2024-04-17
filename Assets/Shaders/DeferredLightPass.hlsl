#include "PBRHelpers.hlsli"
#include "ShadowHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Poisson.hlsli"

#define NUM_THREADS        (16)
#define TOTAL_THREAD_COUNT (NUM_THREADS * NUM_THREADS)

#define BASE_OCCLUSION (0.1f)

// Can be defined from the application
#ifndef MAX_LIGHTS_PER_TILE
    #define MAX_LIGHTS_PER_TILE (1024)
#endif

#ifdef DRAW_TILE_DEBUG 
    #define DRAW_TILE_OCCUPANCY (1)
#else
    #define DRAW_TILE_OCCUPANCY (0)
#endif

//#define DRAW_CASCADE_DEBUG
#ifdef DRAW_CASCADE_DEBUG
    #define DRAW_SHADOW_CASCADE (1)
#else
    #define DRAW_SHADOW_CASCADE (0)
#endif

// G-Buffer
Texture2D<float4> AlbedoTex       : register(t0);
Texture2D<float4> NormalBuffer    : register(t1);
Texture2D<float4> MaterialTex     : register(t2);
Texture2D<float>  DepthStencilTex : register(t3);

// Reflections
Texture2D<float4> DXRReflection : register(t4);

// Reflection probe
TextureCube<float4> IrradianceMap         : register(t5);
TextureCube<float4> SpecularIrradianceMap : register(t6);
Texture2D<float2>   IntegrationLUT        : register(t7);

// Shadow Cascade
Texture2D<float> DirectionalShadowMask : register(t8);

// Point Shadows
TextureCubeArray<float> PointLightShadowMaps : register(t9);

// SSAOBuffer
Texture2D<float> SSAOBuffer : register(t10);

// Shadow Cascade Data - FOR DEBUG
#if DRAW_SHADOW_CASCADE
Texture2D<uint> CascadeIndexBuffer : register(t11);
#endif

// Samplers
SamplerState LUTSampler        : register(s0);
SamplerState IrradianceSampler : register(s1);
SamplerState GBufferSampler    : register(s2);

// Point-Lights
SamplerComparisonState ShadowMapSampler0 : register(s3);

SHADER_CONSTANT_BLOCK_BEGIN
    int NumPointLights;
    int NumShadowCastingPointLights;
    int NumSkyLightMips;
    int ScreenWidth;
    int ScreenHeight;
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

RWTexture2D<float4> Output : register(u0);

// Tiled Light Culling
groupshared uint GroupMinZ;
groupshared uint GroupMaxZ;
groupshared uint GroupPointLightCounter;
groupshared uint GroupPointLightIndices[MAX_LIGHTS_PER_TILE];
groupshared uint GroupShadowPointLightCounter;
groupshared uint GroupShadowPointLightIndices[MAX_LIGHTS_PER_TILE];

float GetNumTilesX()
{
    return DivideByMultiple(Constants.ScreenWidth, NUM_THREADS);
}

float GetNumTilesY()
{
    return DivideByMultiple(Constants.ScreenHeight, NUM_THREADS);
}

float2 GetIntegrationConstants(float NDotV, float Roughness)
{
    return IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, Roughness), 0.0f).rg;
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(FComputeShaderInput Input)
{
    uint ThreadIndex = Input.GroupThreadID.y * NUM_THREADS + Input.GroupThreadID.x;
    if (ThreadIndex == 0)
    {
        GroupMinZ = 0x7f7fffff;
        GroupMaxZ = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();

    uint2 Pixel = Input.DispatchThreadID.xy;
    float Depth     = DepthStencilTex.Load(int3(Pixel, 0));
    float ViewPosZ  = Depth_ProjToView(Depth, CameraBuffer.ProjectionInv);
    
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
        float pxm    = float(NUM_THREADS * Input.GroupID.x);
        float pym    = float(NUM_THREADS * Input.GroupID.y);
        float pxp    = float(NUM_THREADS * (Input.GroupID.x + 1));
        float pyp    = float(NUM_THREADS * (Input.GroupID.y + 1));
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
        GroupPointLightCounter       = 0;
        GroupShadowPointLightCounter = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
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
            InterlockedAdd(GroupPointLightCounter, 1, Index);
            GroupPointLightIndices[Index] = i;
        }
    }
    
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
            InterlockedAdd(GroupShadowPointLightCounter, 1, Index);
            GroupShadowPointLightIndices[Index] = j;
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    // Discard pixels not rendered to the GBuffer
    const float3 GBufferNormal = NormalBuffer.Load(int3(Pixel, 0)).rgb;
    if (length(GBufferNormal) == 0)
    {
        Output[Pixel] = Float4(0.0f);
        return;
    }

    const float2 PixelFloat    = saturate((float2(Pixel) + Float2(0.5f)) / float2(Constants.ScreenWidth, Constants.ScreenHeight));
    const float3 ViewPosition  = PositionFromDepth(Depth, PixelFloat, CameraBuffer.ProjectionInv);
    const float3 WorldPosition = mul(float4(ViewPosition, 1.0), CameraBuffer.ViewInv).xyz;

    const float3 GBufferAlbedo   = saturate(AlbedoTex.Load(int3(Pixel, 0)).rgb);
    const float3 GBufferMaterial = MaterialTex.Load(int3(Pixel, 0)).rgb;

    // Sample with a sampler since the texture is not necessarilly the same size as the screen
    const float ScreenSpaceAO = SSAOBuffer.SampleLevel(GBufferSampler, PixelFloat, 0).r;
    
    const float3 ObjectNormal = UnpackNormal(GBufferNormal);
    const float3 View = normalize(CameraBuffer.Position - WorldPosition);

    const float GBufferRoughness = saturate(GBufferMaterial.r);
    const float GBufferMetallic  = saturate(GBufferMaterial.g);
    const float GBufferAO        = saturate(BASE_OCCLUSION + (GBufferMaterial.b * ScreenSpaceAO));
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, GBufferAlbedo, GBufferMetallic);
    
    float3 L0 = Float3(0.0f);
    
    // Pointlights
    for (uint i = 0; i < GroupPointLightCounter; ++i)
    {
        const int Index = GroupPointLightIndices[i];

        const FPointLight     Light       = PointLights[Index];
        const FPositionRadius LightPosRad = PointLightsPosRad[Index];

        float3 L = LightPosRad.Position - WorldPosition;
        float  DistanceSqrd = dot(L, L);
        float  Attenuation  = 1.0 / max(DistanceSqrd, 0.01 * 0.01);
        L = normalize(L);

        float3 IncidentRadiance = Light.Color * Attenuation;
        IncidentRadiance = DirectRadiance(F0, ObjectNormal, View, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);
            
        L0 += IncidentRadiance;
    }
    
    for (uint i = 0; i < GroupShadowPointLightCounter; i++)
    {
        int Index = GroupShadowPointLightIndices[i];
        const FShadowPointLight Light       = ShadowCastingPointLights[Index];
        const FPositionRadius   LightPosRad = ShadowCastingPointLightsPosRad[Index];
     
        float ShadowFactor = PointLightShadowFactor(PointLightShadowMaps, float(Index), ShadowMapSampler0, WorldPosition, ObjectNormal, Light, LightPosRad);
        if (ShadowFactor > 0.001f)
        {
            float3 L = LightPosRad.Position - WorldPosition;
            float DistanceSqrd = dot(L, L);
            float Attenuation  = 1.0 / max(DistanceSqrd, 0.01 * 0.01);
            L = normalize(L);
            
            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(F0, ObjectNormal, View, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);
            
            L0 += IncidentRadiance * ShadowFactor;
        }
    }
    
    // DirectionalLights
    {    
        const FDirectionalLight Light = DirectionalLightBuffer;
        float3 L = normalize(-Light.Direction);
        
        float ShadowFactor = DirectionalShadowMask.Load(int3(Pixel, 0));
        if (ShadowFactor > 0.0f)
        {
            float3 IncidentRadiance = Light.Color;
            IncidentRadiance = DirectRadiance(F0, ObjectNormal, View, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);      
            L0 += IncidentRadiance * ShadowFactor;
        }
    }
    
    // Image Based Lightning
    float3 FinalColor = L0;
    {
        const float  NDotV      = max(dot(ObjectNormal, View), 0.0f);
        const float3 Reflection = reflect(-View, ObjectNormal);
        
        float3 F  = FresnelSchlick_Roughness(F0, View, ObjectNormal, GBufferRoughness);
        float3 Ks = F;
        float3 Kd = Float3(1.0f) - Ks;
        float3 Irradiance = IrradianceMap.SampleLevel(IrradianceSampler, ObjectNormal, 0.0f).rgb;
        float3 Diffuse    = Irradiance * GBufferAlbedo * Kd;

        float3 PrefilteredMap  = SpecularIrradianceMap.SampleLevel(IrradianceSampler, Reflection, GBufferRoughness * (Constants.NumSkyLightMips - 1.0f)).rgb;
        float2 BRDFIntegration = GetIntegrationConstants(NDotV, GBufferRoughness);
        float3 Specular        = PrefilteredMap * (F * BRDFIntegration.x + BRDFIntegration.y);

        float3 Ambient = (Diffuse + Specular) * GBufferAO;
        FinalColor     = Ambient + L0;
    }

#if DRAW_TILE_OCCUPANCY
    const uint TotalLightCount = GroupPointLightCounter + GroupShadowPointLightCounter;
    
    float4 Tint = Float4(1.0f);
    
    [[branch]]
    if (TotalLightCount > 0)
    {
        if (TotalLightCount < 8)
        {
            float Color = float(TotalLightCount) / 8.0f;
            Tint = float4(0.0f, Color, 0.0f, 1.0f);
        }
        else if (TotalLightCount < 16)
        {
            float Color = float(TotalLightCount) / 16.0f;
            Tint = float4(0.0f, Color, Color, 1.0f);
        }
        else if (TotalLightCount < 32)
        {
            float Color = float(TotalLightCount) / 32.0f;
            Tint = float4(0.0f, 0.0f, Color, 1.0f);
        }
        else if (TotalLightCount < 64)
        {
            float Color = float(TotalLightCount) / 64.0f;
            Tint = float4(Color, Color, 0.0f, 1.0f);
        }
        else
        {
            float Color = float(TotalLightCount) / float(Constants.NumPointLights + Constants.NumShadowCastingPointLights);
            Tint = float4(Color, 0.0f, 0.0f, 1.0f);
        }
    }
    
    FinalColor = FinalColor * Tint.rgb;

#elif DRAW_SHADOW_CASCADE
    const uint CascadeIndex = CascadeIndexBuffer[Pixel];

    float4 Tint = Float4(1.0f);
    if (CascadeIndex == 0)
    {
        Tint = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (CascadeIndex == 1)
    {
        Tint = float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else if (CascadeIndex == 2)
    {
        Tint = float4(0.0f, 0.0f, 1.0f, 1.0f);
    }
    else if (CascadeIndex == 3)
    {
        Tint = float4(1.0f, 1.0f, 0.0f, 1.0f);
    }

    FinalColor = FinalColor * Tint.rgb;
#endif
    
    // Finalize
    float FinalLuminance = Luminance(FinalColor);
    Output[Pixel] = float4(FinalColor, FinalLuminance);
}