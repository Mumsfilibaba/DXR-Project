#pragma once
#include "RendererModule.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"
#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Vector4.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FPointLightData

struct FPointLightData
{
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float    Padding0;
};

MARK_AS_REALLOCATABLE(FPointLightData);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FShadowCastingPointLightData

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FPointLightShadowMapGenerationData

struct FPointLightShadowMapGenerationData
{
    TStaticArray<FMatrix4, 6> Matrix;
    TStaticArray<FMatrix4, 6> ViewMatrix;
    TStaticArray<FMatrix4, 6> ProjMatrix;

    float    FarPlane;
    FVector3 Position;
};

MARK_AS_REALLOCATABLE(FPointLightShadowMapGenerationData);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDirectionalLightData

struct FDirectionalLightData
{
    FVector3 Color      = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias = 0.005f;

    FVector3 Direction     = FVector3(0.0f, -1.0f, 0.0f);
    float    MaxShadowBias = 0.05f;

    FVector3 UpVector = FVector3(0.0f, 0.0f, -1.0f);
    float    LightSize;

    FMatrix4 ShadowMatrix;
};

MARK_AS_REALLOCATABLE(FDirectionalLightData);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FProxyLightProbe

struct FProxyLightProbe
{
    FProxyLightProbe()  = default;
    ~FProxyLightProbe() = default;

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

    FRHITextureCubeRef IrradianceMap;
    FRHITextureCubeRef SpecularIrradianceMap;

    // TODO: We should be able to do this without the UAVs saved
    FRHIUnorderedAccessViewRef         IrradianceMapUAV;
    TArray<FRHIUnorderedAccessViewRef> SpecularIrradianceMapUAVs;
    TArray<FRHIUnorderedAccessView*>   WeakSpecularIrradianceMapUAVs;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FLightSetup

struct RENDERER_API FLightSetup
{
    const EFormat ShadowMaskFormat = EFormat::R8_Unorm;
    const EFormat ShadowMapFormat  = EFormat::D16_Unorm;
    const EFormat LightProbeFormat = EFormat::R11G11B10_Float;

    const uint32 MaxPointLights         = 256;
    const uint32 MaxDirectionalLights   = 256;
    const uint32 MaxPointLightShadows   = 8;

    const uint16 CascadeSize            = 2048;

    const uint16 IrradianceSize         = 32;
    const uint16 SpecularIrradianceSize = 256;
    const uint16 PointLightShadowSize   = 512;

    FLightSetup()  = default;
    ~FLightSetup() = default;

    bool Init();
    void Release();

    void BeginFrame(FRHICommandList& CommandList, const FScene& Scene);

    // PointLights
    TArray<FVector4>        PointLightsPosRad;
    TArray<FPointLightData> PointLightsData;

    FRHIConstantBufferRef   PointLightsBuffer;
    FRHIConstantBufferRef   PointLightsPosRadBuffer;

    TArray<FPointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;

    TArray<FVector4>                     ShadowCastingPointLightsPosRad;
    TArray<FShadowCastingPointLightData> ShadowCastingPointLightsData;

    FRHIConstantBufferRef                ShadowCastingPointLightsBuffer;
    FRHIConstantBufferRef                ShadowCastingPointLightsPosRadBuffer;

    FRHITextureCubeArrayRef              PointLightShadowMaps;

    // DirectionalLight
    // NOTE: Only one directional light (TODO: This is ugly)
    FDirectionalLightData      DirectionalLightData;
    float                      CascadeSplitLambda;
    bool                       DirectionalLightDataDirty = true;

    FRHIConstantBufferRef      DirectionalLightsBuffer;

    FRHITexture2DRef           ShadowMapCascades[4];
    FRHITexture2DRef           DirectionalShadowMask;
    FRHITexture2DRef           CascadeIndexBuffer;

    FRHIGenericBufferRef       CascadeMatrixBuffer;
    FRHIShaderResourceViewRef  CascadeMatrixBufferSRV;
    FRHIUnorderedAccessViewRef CascadeMatrixBufferUAV;

    FRHIGenericBufferRef       CascadeSplitsBuffer;
    FRHIShaderResourceViewRef  CascadeSplitsBufferSRV;
    FRHIUnorderedAccessViewRef CascadeSplitsBufferUAV;

    // SkyLight
    FProxyLightProbe Skylight;
    FProxyLightProbe LocalProbe;
};
