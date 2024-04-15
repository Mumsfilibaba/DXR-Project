#pragma once
#include "RendererModule.h"
#include "Core/Math/Vector4.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "Engine/World/World.h"
#include "Engine/World/Lights/DirectionalLight.h"

#define MAX_LIGHTS_PER_TILE (1024)
#define NUM_DEFAULT_SHADOW_CASTING_POINT_LIGHTS (8)

struct FPointLightDataHLSL
{
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float    Padding0;
};

MARK_AS_REALLOCATABLE(FPointLightDataHLSL);

struct FShadowCastingPointLightDataHLSL
{
    // 0-16
    FVector3 Color         = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias    = 0.005f;
    // 16-24
    float    FarPlane      = 10.0f;
    float    MaxShadowBias = 0.05f;
    // 24-32
    float    Padding0;
    float    Padding1;
};

MARK_AS_REALLOCATABLE(FShadowCastingPointLightDataHLSL);

struct FDirectionalLightDataHLSL
{
    // 0-16
    FVector3 Color         = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias    = 0.005f;
    // 16-32
    FVector3 Direction     = FVector3(0.0f, -1.0f, 0.0f);
    float    MaxShadowBias = 0.05f;
    // 32-48
    FVector3 UpVector      = FVector3(0.0f, 0.0f, -1.0f);
    float    LightSize     = 0.0f;
    // 48-112
    FMatrix4 ShadowMatrix;
};

MARK_AS_REALLOCATABLE(FDirectionalLightDataHLSL);

struct FCascadeGenerationInfoHLSL
{
    // 0-64
    FMatrix4 ShadowMatrix;
    // 64-80
    FVector3 LightDirection;
    float    CascadeSplitLambda;
    // 80-96
    FVector3 LightUp;
    float    CascadeResolution;
    // 96-112
    int32    bDepthReductionEnabled;
    int32    MaxCascadeIndex;
    int32    Padding0;
    int32    Padding1;
};

MARK_AS_REALLOCATABLE(FCascadeGenerationInfoHLSL);

struct FProxyLightProbe
{
    void Release()
    {
        IrradianceMap.Reset();
        SpecularIrradianceMap.Reset();

        IrradianceMapUAV.Reset();
        for (FRHIUnorderedAccessViewRef& UAV : SpecularIrradianceMapUAVs)
        {
            UAV.Reset();
        }

        SpecularIrradianceMapUAVs.Clear();
        WeakSpecularIrradianceMapUAVs.Clear();
    }

    FRHITextureRef IrradianceMap;
    FRHITextureRef SpecularIrradianceMap;

    // TODO: We should be able to do this without the UAVs saved
    FRHIUnorderedAccessViewRef         IrradianceMapUAV;
    TArray<FRHIUnorderedAccessViewRef> SpecularIrradianceMapUAVs;
    TArray<FRHIUnorderedAccessView*>   WeakSpecularIrradianceMapUAVs;
};


struct FLightSetup
{
    FLightSetup()
        : DirectionalLightDataDirty(true)
        , CascadeSplitLambda(0.0f)
        , CascadeGenerationDataDirty(true)
    {
    }

    bool Initialize();
    void Release();

    void BeginFrame(FRHICommandList& CommandList, FScene* Scene);

    const EFormat ShadowMaskFormat    = EFormat::R8_Unorm;
    const EFormat ShadowMapFormat     = EFormat::D16_Unorm;
    const EFormat LightProbeFormat    = EFormat::R11G11B10_Float;

    const uint32 MaxPointLights       = 256;
    const uint32 MaxDirectionalLights = 256;
    const uint32 MaxPointLightShadows = 8;

    int32 CascadeSize                 = 0;
    int32 PointLightShadowSize        = 512;
    int32 IrradianceProbeSize         = 0;
    int32 SpecularIrradianceProbeSize = 0;

    // PointLights
    TArray<FVector4>                         PointLightsPosRad;
    TArray<FPointLightDataHLSL>              PointLightsData;
    FRHIBufferRef                            PointLightsBuffer;
    FRHIBufferRef                            PointLightsPosRadBuffer;

    TArray<FVector4>                         ShadowCastingPointLightsPosRad;
    TArray<FShadowCastingPointLightDataHLSL> ShadowCastingPointLightsData;
    FRHIBufferRef                            ShadowCastingPointLightsBuffer;
    FRHIBufferRef                            ShadowCastingPointLightsPosRadBuffer;
    FRHITextureRef                           PointLightShadowMaps;

    // DirectionalLight
    // NOTE: Only one directional light
    FDirectionalLightDataHLSL  DirectionalLightData;
    bool                       DirectionalLightDataDirty;
    float                      CascadeSplitLambda;

    FRHIBufferRef              DirectionalLightDataBuffer;

    FCascadeGenerationInfoHLSL CascadeGenerationData;
    bool                       CascadeGenerationDataDirty;
    FRHIBufferRef              CascadeGenerationDataBuffer;

    FRHITextureRef             ShadowMapCascades[4];
    FRHITextureRef             DirectionalShadowMask;
    FRHITextureRef             CascadeIndexBuffer;

    FRHIBufferRef              CascadeMatrixBuffer;
    FRHIShaderResourceViewRef  CascadeMatrixBufferSRV;
    FRHIUnorderedAccessViewRef CascadeMatrixBufferUAV;

    FRHIBufferRef              CascadeSplitsBuffer;
    FRHIShaderResourceViewRef  CascadeSplitsBufferSRV;
    FRHIUnorderedAccessViewRef CascadeSplitsBufferUAV;

    // SkyLight
    FProxyLightProbe           Skylight;
    FProxyLightProbe           LocalProbe;
};
