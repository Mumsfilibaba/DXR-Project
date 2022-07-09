#pragma once
#include "RendererModule.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"
#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Vector4.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SPointLightData

struct SPointLightData
{
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float Padding0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShadowCastingPointLightData

struct SShadowCastingPointLightData
{
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float ShadowBias = 0.005f;

    float FarPlane = 10.0f;
    float MaxShadowBias = 0.05f;
    float Padding0;
    float Padding1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SPointLightShadowMapGenerationData

struct SPointLightShadowMapGenerationData
{
    TStaticArray<FMatrix4, 6> Matrix;
    TStaticArray<FMatrix4, 6> ViewMatrix;
    TStaticArray<FMatrix4, 6> ProjMatrix;

    float    FarPlane;
    FVector3 Position;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SDirectionalLightData

struct SDirectionalLightData
{
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float ShadowBias = 0.005f;

    FVector3 Direction = FVector3(0.0f, -1.0f, 0.0f);
    float MaxShadowBias = 0.05f;

    FVector3 Up = FVector3(0.0f, 0.0f, -1.0f);
    float LightSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SLightSetup

struct RENDERER_API SLightSetup
{
    const EFormat ShadowMaskFormat = EFormat::R8_Unorm;
    const EFormat ShadowMapFormat  = EFormat::D16_Unorm;
    const EFormat LightProbeFormat = EFormat::R11G11B10_Float;

    const uint32 MaxPointLights       = 256;
    const uint32 MaxDirectionalLights = 256;
    const uint32 MaxPointLightShadows = 8;

    const uint16 CascadeSize = 2048;

    const uint16 IrradianceSize         = 32;
    const uint16 SpecularIrradianceSize = 256;
    const uint16 PointLightShadowSize   = 512;

    SLightSetup() = default;
    ~SLightSetup() = default;

    bool Init();

    void BeginFrame(FRHICommandList& CmdList, const FScene& Scene);
    void Release();

    TArray<FVector4>        PointLightsPosRad;
    TArray<SPointLightData> PointLightsData;

    TSharedRef<FRHIConstantBuffer> PointLightsBuffer;
    TSharedRef<FRHIConstantBuffer> PointLightsPosRadBuffer;

    TArray<SPointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;

    TArray<FVector4>                     ShadowCastingPointLightsPosRad;
    TArray<SShadowCastingPointLightData> ShadowCastingPointLightsData;

    TSharedRef<FRHIConstantBuffer> ShadowCastingPointLightsBuffer;
    TSharedRef<FRHIConstantBuffer> ShadowCastingPointLightsPosRadBuffer;

    TSharedRef<FRHITextureCubeArray> PointLightShadowMaps;

    // NOTE: Only one directional light
    SDirectionalLightData DirectionalLightData;
    bool DirectionalLightDataDirty = true;

    float CascadeSplitLambda;

    TSharedRef<FRHIConstantBuffer> DirectionalLightsBuffer;

    FRHITexture2DRef ShadowMapCascades[4];
    FRHITexture2DRef DirectionalShadowMask;

    TSharedRef<FRHIGenericBuffer>       CascadeMatrixBuffer;
    TSharedRef<FRHIShaderResourceView>  CascadeMatrixBufferSRV;
    FRHIUnorderedAccessViewRef CascadeMatrixBufferUAV;

    TSharedRef<FRHIGenericBuffer>       CascadeSplitsBuffer;
    TSharedRef<FRHIShaderResourceView>  CascadeSplitsBufferSRV;
    FRHIUnorderedAccessViewRef CascadeSplitsBufferUAV;

    FRHITextureCubeRef         IrradianceMap;
    FRHIUnorderedAccessViewRef IrradianceMapUAV;

    FRHITextureCubeRef                 SpecularIrradianceMap;
    TArray<FRHIUnorderedAccessViewRef> SpecularIrradianceMapUAVs;
    TArray<FRHIUnorderedAccessView*>            WeakSpecularIrradianceMapUAVs;
};
