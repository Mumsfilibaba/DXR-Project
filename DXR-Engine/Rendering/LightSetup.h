#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/ResourceViews.h"
#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

#define NUM_SHADOW_CASCADES (4)

struct PointLightData
{
    XMFLOAT3 Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    float Padding0;
};

struct ShadowCastingPointLightData
{
    XMFLOAT3 Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    float    ShadowBias    = 0.005f;
    float    FarPlane      = 10.0f;
    float    MaxShadowBias = 0.05f;
    float Padding0;
    float Padding1;
};

struct PointLightShadowMapGenerationData
{
    TStaticArray<XMFLOAT4X4, 6> Matrix;
    TStaticArray<XMFLOAT4X4, 6> ViewMatrix;
    TStaticArray<XMFLOAT4X4, 6> ProjMatrix;
    float    FarPlane;
    XMFLOAT3 Position;
};

struct DirectionalLightData
{
    XMFLOAT3   Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    float      ShadowBias    = 0.005f;
    XMFLOAT3   Direction     = XMFLOAT3(0.0f, -1.0f, 0.0f);
    float      MaxShadowBias = 0.05f;
    XMFLOAT4X4 LightMatrix;
};

struct DirLightShadowMapGenerationData
{
    XMFLOAT4X4 Matrix;
    float    FarPlane;

    float ShadowCascadesFarPlanes[NUM_SHADOW_CASCADES + 1];
    XMFLOAT4X4 CascadeMatrices[NUM_SHADOW_CASCADES];

    XMFLOAT3 Position;
};

struct LightSetup
{
    const EFormat ShadowMapFormat  = EFormat::D32_Float;
    const EFormat LightProbeFormat = EFormat::R16G16B16A16_Float;
    
    const uint32 MaxPointLights       = 256;
    const uint32 MaxDirectionalLights = 256;
    const uint32 MaxPointLightShadows = 8;
    
    const uint16 ShadowMapWidth  = 4096;
    const uint16 ShadowMapHeight = 4096;

    const uint16 CascadeWidth  = 2048;
    const uint16 CascadeHeight = 2048;
    
    const uint16 PointLightShadowSize = 1024;

    LightSetup()  = default;
    ~LightSetup() = default;

    bool Init();

    void BeginFrame(CommandList& CmdList, const Scene& Scene);
    void Release();

    TArray<XMFLOAT4>       PointLightsPosRad;
    TArray<PointLightData> PointLightsData;
    
    TRef<ConstantBuffer> PointLightsBuffer;
    TRef<ConstantBuffer> PointLightsPosRadBuffer;

    TArray<PointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;
    
    TArray<XMFLOAT4>                    ShadowCastingPointLightsPosRad;
    TArray<ShadowCastingPointLightData> ShadowCastingPointLightsData;
    
    TRef<ConstantBuffer> ShadowCastingPointLightsBuffer;
    TRef<ConstantBuffer> ShadowCastingPointLightsPosRadBuffer;
    
    TRef<TextureCubeArray>       PointLightShadowMaps;
    TArray<DepthStencilViewCube> PointLightShadowMapDSVs;

    TArray<DirLightShadowMapGenerationData> DirLightShadowMapsGenerationData;
    TArray<DirectionalLightData>            DirectionalLightsData;
    
    TRef<ConstantBuffer> DirectionalLightsBuffer;
    
    TRef<Texture2D> DirLightShadowMap;
    TRef<Texture2D> ShadowMapCascades[4];

    TRef<TextureCube>         IrradianceMap;
    TRef<UnorderedAccessView> IrradianceMapUAV;

    TRef<TextureCube>                 SpecularIrradianceMap;
    TArray<TRef<UnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<UnorderedAccessView*>      WeakSpecularIrradianceMapUAVs;
};