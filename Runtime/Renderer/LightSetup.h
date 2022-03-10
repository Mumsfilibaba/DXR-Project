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
    CVector3 Color = CVector3(1.0f, 1.0f, 1.0f);
    float Padding0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShadowCastingPointLightData

struct SShadowCastingPointLightData
{
    CVector3 Color = CVector3(1.0f, 1.0f, 1.0f);
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
    TStaticArray<CMatrix4, 6> Matrix;
    TStaticArray<CMatrix4, 6> ViewMatrix;
    TStaticArray<CMatrix4, 6> ProjMatrix;

    float    FarPlane;
    CVector3 Position;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SDirectionalLightData

struct SDirectionalLightData
{
    CVector3 Color = CVector3(1.0f, 1.0f, 1.0f);
    float ShadowBias = 0.005f;

    CVector3 Direction = CVector3(0.0f, -1.0f, 0.0f);
    float MaxShadowBias = 0.05f;

    CVector3 Up = CVector3(0.0f, 0.0f, -1.0f);
    float LightSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SLightSetup

struct RENDERER_API SLightSetup
{
    const ERHIFormat ShadowMaskFormat = ERHIFormat::R8_Unorm;
    const ERHIFormat ShadowMapFormat = ERHIFormat::D32_Float;
    const ERHIFormat LightProbeFormat = ERHIFormat::R11G11B10_Float;

    const uint32 MaxPointLights = 256;
    const uint32 MaxDirectionalLights = 256;
    const uint32 MaxPointLightShadows = 8;

    const uint16 CascadeSize = 4096;

    const uint16 IrradianceSize = 32;
    const uint16 SpecularIrradianceSize = 256;
    const uint16 PointLightShadowSize = 512;

    SLightSetup() = default;
    ~SLightSetup() = default;

    bool Init();

    void BeginFrame(CRHICommandList& CmdList, const CScene& Scene);
    void Release();

    TArray<CVector4>        PointLightsPosRad;
    TArray<SPointLightData> PointLightsData;

    TSharedRef<CRHIBuffer> PointLightsBuffer;
    TSharedRef<CRHIBuffer> PointLightsPosRadBuffer;

    TArray<SPointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;

    TArray<CVector4>                     ShadowCastingPointLightsPosRad;
    TArray<SShadowCastingPointLightData> ShadowCastingPointLightsData;

    TSharedRef<CRHIBuffer> ShadowCastingPointLightsBuffer;
    TSharedRef<CRHIBuffer> ShadowCastingPointLightsPosRadBuffer;

    TSharedRef<CRHITextureCubeArray> PointLightShadowMaps;
    TArray<DepthStencilViewCube>     PointLightShadowMapDSVs;

    // NOTE: Only one directional light
    SDirectionalLightData DirectionalLightData;
    bool DirectionalLightDataDirty = true;

    float CascadeSplitLambda;

    TSharedRef<CRHIBuffer> DirectionalLightsBuffer;

    TSharedRef<CRHITexture2D> ShadowMapCascades[4];
    TSharedRef<CRHITexture2D> DirectionalShadowMask;

    TSharedRef<CRHIBuffer>    CascadeMatrixBuffer;
    TSharedRef<CRHIShaderResourceView>  CascadeMatrixBufferSRV;
    TSharedRef<CRHIUnorderedAccessView> CascadeMatrixBufferUAV;

    TSharedRef<CRHIBuffer>    CascadeSplitsBuffer;
    TSharedRef<CRHIShaderResourceView>  CascadeSplitsBufferSRV;
    TSharedRef<CRHIUnorderedAccessView> CascadeSplitsBufferUAV;

    TSharedRef<CRHITextureCube>         IrradianceMap;
    TSharedRef<CRHIUnorderedAccessView> IrradianceMapUAV;

    TSharedRef<CRHITextureCube>                 SpecularIrradianceMap;
    TArray<TSharedRef<CRHIUnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<CRHIUnorderedAccessView*>            WeakSpecularIrradianceMapUAVs;
};