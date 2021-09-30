#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/ResourceViews.h"
#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Core/Math/Vector4.h"

struct PointLightData
{
    CVector3 Color = CVector3( 1.0f, 1.0f, 1.0f );
    float Padding0;
};

struct ShadowCastingPointLightData
{
    CVector3 Color = CVector3( 1.0f, 1.0f, 1.0f );
    float ShadowBias = 0.005f;
    float FarPlane = 10.0f;
    float MaxShadowBias = 0.05f;
    float Padding0;
    float Padding1;
};

struct PointLightShadowMapGenerationData
{
    TStaticArray<CMatrix4, 6> Matrix;
    TStaticArray<CMatrix4, 6> ViewMatrix;
    TStaticArray<CMatrix4, 6> ProjMatrix;
    float    FarPlane;
    CVector3 Position;
};

struct DirectionalLightData
{
    CVector3 Color = CVector3( 1.0f, 1.0f, 1.0f );
    float ShadowBias = 0.005f;
    CVector3 Direction = CVector3( 0.0f, -1.0f, 0.0f );
    float MaxShadowBias = 0.05f;
    CVector3 Up = CVector3( 0.0f, 0.0f, -1.0f );
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

    void BeginFrame( CommandList& CmdList, const CScene& Scene );
    void Release();

    TArray<CVector4>       PointLightsPosRad;
    TArray<PointLightData> PointLightsData;

    TSharedRef<ConstantBuffer> PointLightsBuffer;
    TSharedRef<ConstantBuffer> PointLightsPosRadBuffer;

    TArray<PointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;

    TArray<CVector4>                    ShadowCastingPointLightsPosRad;
    TArray<ShadowCastingPointLightData> ShadowCastingPointLightsData;

    TSharedRef<ConstantBuffer> ShadowCastingPointLightsBuffer;
    TSharedRef<ConstantBuffer> ShadowCastingPointLightsPosRadBuffer;

    TSharedRef<TextureCubeArray>       PointLightShadowMaps;
    TArray<DepthStencilViewCube> PointLightShadowMapDSVs;

    // NOTE: Only one directional light
    DirectionalLightData DirectionalLightData;
    bool DirectionalLightDataDirty = true;

    float CascadeSplitLambda;

    TSharedRef<ConstantBuffer> DirectionalLightsBuffer;

    TSharedRef<Texture2D> ShadowMapCascades[4];
    TSharedRef<Texture2D> DirectionalShadowMask;

    TSharedRef<StructuredBuffer>    CascadeMatrixBuffer;
    TSharedRef<ShaderResourceView>  CascadeMatrixBufferSRV;
    TSharedRef<UnorderedAccessView> CascadeMatrixBufferUAV;

    TSharedRef<StructuredBuffer>    CascadeSplitsBuffer;
    TSharedRef<ShaderResourceView>  CascadeSplitsBufferSRV;
    TSharedRef<UnorderedAccessView> CascadeSplitsBufferUAV;

    TSharedRef<TextureCube>         IrradianceMap;
    TSharedRef<UnorderedAccessView> IrradianceMapUAV;

    TSharedRef<TextureCube>                 SpecularIrradianceMap;
    TArray<TSharedRef<UnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<UnorderedAccessView*>      WeakSpecularIrradianceMapUAVs;
};