#pragma once
#include "RendererModule.h"
#include "Core/Math/Vector4.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#define MAX_LIGHTS_PER_TILE (1024)

struct FPointLightData
{
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float    Padding0;
};

MARK_AS_REALLOCATABLE(FPointLightData);


struct FShadowCastingPointLightData
{
    FVector3 Color      = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias = 0.005f;

    float    FarPlane      = 10.0f;
    float    MaxShadowBias = 0.05f;
    float    Padding0;
    float    Padding1;
};

MARK_AS_REALLOCATABLE(FShadowCastingPointLightData);


struct FPointLightShadowMapGenerationData
{
    TStaticArray<FMatrix4, 6> Matrix;
    TStaticArray<FMatrix4, 6> ViewMatrix;
    TStaticArray<FMatrix4, 6> ProjMatrix;

    FVector3 Position;
    float    FarPlane;
    int32    LightIndex;
};

MARK_AS_REALLOCATABLE(FPointLightShadowMapGenerationData);


struct FDirectionalLightData
{
    FVector3 Color      = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias = 0.005f;

    FVector3 Direction     = FVector3(0.0f, -1.0f, 0.0f);
    float    MaxShadowBias = 0.05f;

    FVector3 UpVector = FVector3(0.0f, 0.0f, -1.0f);
    float    LightSize = 0.0f;

    FMatrix4 ShadowMatrix;
};

MARK_AS_REALLOCATABLE(FDirectionalLightData);


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


struct RENDERER_API FLightSetup
{
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

    bool Initialize();
    void Release();

    void BeginFrame(FRHICommandList& CommandList, FRendererScene* Scene);

    // PointLights
    TArray<FVector4>                           PointLightsPosRad;
    TArray<FPointLightData>                    PointLightsData;
    FRHIBufferRef                              PointLightsBuffer;
    FRHIBufferRef                              PointLightsPosRadBuffer;

    TArray<FPointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;
    TArray<FVector4>                           ShadowCastingPointLightsPosRad;
    TArray<FShadowCastingPointLightData>       ShadowCastingPointLightsData;
    FRHIBufferRef                              ShadowCastingPointLightsBuffer;
    FRHIBufferRef                              ShadowCastingPointLightsPosRadBuffer;
    FRHITextureRef                             PointLightShadowMaps;

    // DirectionalLight
    // NOTE: Only one directional light
    FDirectionalLightData      DirectionalLightData;

    FMatrix4                   DirectionalLightViewMatrix;
    FMatrix4                   DirectionalLightProjMatrix;

    float                      DirectionalLightFarPlane = 0.0f;
    float                      CascadeSplitLambda       = 0.0f;
    
    bool                       DirectionalLightDataDirty = true;

    FRHIBufferRef              DirectionalLightsBuffer;

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
    FProxyLightProbe Skylight;
    FProxyLightProbe LocalProbe;
};
