#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/ResourceViews.h"
#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"
#include "Scene/Lights/DirectionalLight.h"

struct PointLightData
{
    XMFLOAT3 Color = XMFLOAT3( 1.0f, 1.0f, 1.0f );
    float Padding0;
};

struct ShadowCastingPointLightData
{
    XMFLOAT3 Color = XMFLOAT3( 1.0f, 1.0f, 1.0f );
    float ShadowBias = 0.005f;
    float FarPlane = 10.0f;
    float MaxShadowBias = 0.05f;
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
    XMFLOAT3 Color = XMFLOAT3( 1.0f, 1.0f, 1.0f );
    float ShadowBias = 0.005f;
    XMFLOAT3 Direction = XMFLOAT3( 0.0f, -1.0f, 0.0f );
    float MaxShadowBias = 0.05f;
    XMFLOAT3 Up = XMFLOAT3( 0.0f, 0.0f, -1.0f );
    float LightSize;
};

struct LightSetup
{
    const EFormat ShadowMapFormat = EFormat::D32_Float;
    const EFormat LightProbeFormat = EFormat::R11G11B10_Float;

    const uint32 MaxPointLights = 256;
    const uint32 MaxDirectionalLights = 256;
    const uint32 MaxPointLightShadows = 8;

    const uint16 CascadeSize = 4096;

    const uint16 IrradianceSize = 32;
    const uint16 SpecularIrradianceSize = 256;
    const uint16 PointLightShadowSize = 512;

    LightSetup() = default;
    ~LightSetup() = default;

    bool Init();

    void BeginFrame( CommandList& CmdList, const Scene& Scene );
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

    // NOTE: Only one directional light
    DirectionalLightData DirectionalLightData;
    bool DirectionalLightDataDirty = true;

    float CascadeSplitLambda;

    TRef<ConstantBuffer> DirectionalLightsBuffer;

    TRef<Texture2D> ShadowMapCascades[4];
    TRef<Texture2D> DirectionalShadowMask;

    TRef<StructuredBuffer>    CascadeMatrixBuffer;
    TRef<ShaderResourceView>  CascadeMatrixBufferSRV;
    TRef<UnorderedAccessView> CascadeMatrixBufferUAV;

    TRef<StructuredBuffer>    CascadeSplitsBuffer;
    TRef<ShaderResourceView>  CascadeSplitsBufferSRV;
    TRef<UnorderedAccessView> CascadeSplitsBufferUAV;

    TRef<TextureCube>         IrradianceMap;
    TRef<UnorderedAccessView> IrradianceMapUAV;

    TRef<TextureCube>                 SpecularIrradianceMap;
    TArray<TRef<UnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<UnorderedAccessView*>      WeakSpecularIrradianceMapUAVs;
};