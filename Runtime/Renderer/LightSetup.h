#pragma once
#include "RendererAPI.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIResourceViews.h"
#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Vector4.h"

struct SPointLightData
{
    CVector3 Color = CVector3( 1.0f, 1.0f, 1.0f );
    float Padding0;
};

struct SShadowCastingPointLightData
{
    CVector3 Color = CVector3( 1.0f, 1.0f, 1.0f );
    float ShadowBias = 0.005f;

    float FarPlane = 10.0f;
    float MaxShadowBias = 0.05f;
    float Padding0;
    float Padding1;
};

struct SPointLightShadowMapGenerationData
{
    TStaticArray<CMatrix4, 6> Matrix;
    TStaticArray<CMatrix4, 6> ViewMatrix;
    TStaticArray<CMatrix4, 6> ProjMatrix;

    float    FarPlane;
    CVector3 Position;
};

struct SDirectionalLightData
{
    CVector3 Color = CVector3( 1.0f, 1.0f, 1.0f );
    float ShadowBias = 0.005f;

    CVector3 Direction = CVector3( 0.0f, -1.0f, 0.0f );
    float MaxShadowBias = 0.05f;

    CVector3 Up = CVector3( 0.0f, 0.0f, -1.0f );
    float LightSize;
};

struct RENDERER_API SLightSetup
{
    const EFormat ShadowMaskFormat = EFormat::R8_Unorm;
    const EFormat ShadowMapFormat = EFormat::D32_Float;
    const EFormat LightProbeFormat = EFormat::R11G11B10_Float;

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

    void BeginFrame( CRHICommandList& CmdList, const CScene& Scene );
    void Release();

    TArray<CVector4>        PointLightsPosRad;
    TArray<SPointLightData> PointLightsData;

    TSharedRef<CRHIConstantBuffer> PointLightsBuffer;
    TSharedRef<CRHIConstantBuffer> PointLightsPosRadBuffer;

    TArray<SPointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;

    TArray<CVector4>                     ShadowCastingPointLightsPosRad;
    TArray<SShadowCastingPointLightData> ShadowCastingPointLightsData;

    TSharedRef<CRHIConstantBuffer> ShadowCastingPointLightsBuffer;
    TSharedRef<CRHIConstantBuffer> ShadowCastingPointLightsPosRadBuffer;

    TSharedRef<CRHITextureCubeArray> PointLightShadowMaps;
    TArray<DepthStencilViewCube>     PointLightShadowMapDSVs;

    // NOTE: Only one directional light
    SDirectionalLightData DirectionalLightData;
    bool DirectionalLightDataDirty = true;

    float CascadeSplitLambda;

    TSharedRef<CRHIConstantBuffer> DirectionalLightsBuffer;

    TSharedRef<CRHITexture2D> ShadowMapCascades[4];
    TSharedRef<CRHITexture2D> DirectionalShadowMask;

    TSharedRef<CRHIStructuredBuffer>    CascadeMatrixBuffer;
    TSharedRef<CRHIShaderResourceView>  CascadeMatrixBufferSRV;
    TSharedRef<CRHIUnorderedAccessView> CascadeMatrixBufferUAV;

    TSharedRef<CRHIStructuredBuffer>    CascadeSplitsBuffer;
    TSharedRef<CRHIShaderResourceView>  CascadeSplitsBufferSRV;
    TSharedRef<CRHIUnorderedAccessView> CascadeSplitsBufferUAV;

    TSharedRef<CRHITextureCube>         IrradianceMap;
    TSharedRef<CRHIUnorderedAccessView> IrradianceMapUAV;

    TSharedRef<CRHITextureCube>                 SpecularIrradianceMap;
    TArray<TSharedRef<CRHIUnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<CRHIUnorderedAccessView*>            WeakSpecularIrradianceMapUAVs;
};