#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/ResourceViews.h"
#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

struct PointLightData
{
    XMFLOAT3 Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float Padding0;
};

struct ShadowCastingPointLightData
{
    XMFLOAT3 Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float    ShadowBias    = 0.005f;
    Float    FarPlane      = 10.0f;
    Float    MaxShadowBias = 0.05f;
    Float Padding0;
    Float Padding1;
};

struct PointLightShadowMapGenerationData
{
    TStaticArray<XMFLOAT4X4, 6> Matrix;
    TStaticArray<XMFLOAT4X4, 6> ViewMatrix;
    TStaticArray<XMFLOAT4X4, 6> ProjMatrix;
    Float    FarPlane;
    XMFLOAT3 Position;
};

struct DirectionalLightData
{
    XMFLOAT3   Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float      ShadowBias    = 0.005f;
    XMFLOAT3   Direction     = XMFLOAT3(0.0f, -1.0f, 0.0f);
    Float      MaxShadowBias = 0.05f;
    XMFLOAT4X4 LightMatrix;
};

struct DirLightShadowMapGenerationData
{
    XMFLOAT4X4 Matrix;
    Float      FarPlane;
    XMFLOAT3   Position;
};

struct LightSetup
{
    const EFormat ShadowMapFormat      = EFormat::D32_Float;
    const EFormat LightProbeFormat     = EFormat::R16G16B16A16_Float;
    const UInt32  MaxPointLights       = 256;
    const UInt32  MaxDirectionalLights = 256;
    const UInt32  MaxPointLightShadows = 8;
    const UInt16  ShadowMapWidth       = 4096;
    const UInt16  ShadowMapHeight      = 4096;
    const UInt16  PointLightShadowSize = 1024;

    LightSetup()  = default;
    ~LightSetup() = default;

    Bool Init();

    void BeginFrame(CommandList& CmdList, const Scene& Scene);
    void Release();

    TArray<XMFLOAT4>           PointLightsPosRad;
    TArray<PointLightData>     PointLightsData;
    TSharedRef<ConstantBuffer> PointLightsBuffer;
    TSharedRef<ConstantBuffer> PointLightsPosRadBuffer;

    TArray<PointLightShadowMapGenerationData> PointLightShadowMapsGenerationData;
    TArray<XMFLOAT4>                    ShadowCastingPointLightsPosRad;
    TArray<ShadowCastingPointLightData> ShadowCastingPointLightsData;
    TSharedRef<ConstantBuffer>          ShadowCastingPointLightsBuffer;
    TSharedRef<ConstantBuffer>          ShadowCastingPointLightsPosRadBuffer;
    TSharedRef<TextureCubeArray>        PointLightShadowMaps;
    TArray<DepthStencilViewCube>        PointLightShadowMapDSVs;

    TArray<DirLightShadowMapGenerationData> DirLightShadowMapsGenerationData;
    TArray<DirectionalLightData> DirectionalLightsData;
    TSharedRef<ConstantBuffer>   DirectionalLightsBuffer;
    TSharedRef<Texture2D>        DirLightShadowMaps;

    TSharedRef<TextureCube>         IrradianceMap;
    TSharedRef<UnorderedAccessView> IrradianceMapUAV;

    TSharedRef<TextureCube>                 SpecularIrradianceMap;
    TArray<TSharedRef<UnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<UnorderedAccessView*>            WeakSpecularIrradianceMapUAVs;
};