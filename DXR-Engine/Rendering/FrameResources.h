#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/Viewport.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/DebugUI.h"

#define GBUFFER_ALBEDO_INDEX      0
#define GBUFFER_NORMAL_INDEX      1
#define GBUFFER_MATERIAL_INDEX    2
#define GBUFFER_DEPTH_INDEX       3
#define GBUFFER_VIEW_NORMAL_INDEX 4

struct FrameResources
{
    FrameResources()  = default;
    ~FrameResources() = default;

    void Release();

    const EFormat DepthBufferFormat  = EFormat::D32_Float;
    const EFormat SSAOBufferFormat   = EFormat::R16_Float;
    const EFormat FinalTargetFormat  = EFormat::R16G16B16A16_Float;
    const EFormat RenderTargetFormat = EFormat::R8G8B8A8_Unorm;
    const EFormat AlbedoFormat       = EFormat::R8G8B8A8_Unorm;
    const EFormat MaterialFormat     = EFormat::R8G8B8A8_Unorm;
    const EFormat NormalFormat       = EFormat::R10G10B10A2_Unorm;
    const EFormat ViewNormalFormat   = EFormat::R10G10B10A2_Unorm;

    Texture2D* BackBuffer = nullptr;

    TSharedRef<ConstantBuffer> CameraBuffer;
    TSharedRef<ConstantBuffer> TransformBuffer;

    TSharedRef<SamplerState> DirectionalShadowSampler;
    TSharedRef<SamplerState> PointShadowSampler;
    TSharedRef<SamplerState> IrradianceSampler;

    TSharedRef<TextureCube> Skybox;

    TSharedRef<Texture2D>    IntegrationLUT;
    TSharedRef<SamplerState> IntegrationLUTSampler;

    TSharedRef<Texture2D>    ReflectionTexture;
    TSharedRef<Texture2D>    SSAOBuffer;
    TSharedRef<Texture2D>    FinalTarget;
    TSharedRef<Texture2D>    GBuffer[5];
    TSharedRef<SamplerState> GBufferSampler;
    TSharedRef<SamplerState> FXAASampler;

    TSharedRef<InputLayoutState> StdInputLayout;

    TArray<MeshDrawCommand> DeferredVisibleCommands;
    TArray<MeshDrawCommand> ForwardVisibleCommands;

    TArray<ImGuiImage> DebugTextures;

    TSharedRef<Viewport> MainWindowViewport;
};

using DepthStencilViewCube = TStaticArray<TSharedRef<DepthStencilView>, 6>;

struct SceneLightSetup
{
    SceneLightSetup()  = default;
    ~SceneLightSetup() = default;

    void Release();

    const EFormat ShadowMapFormat      = EFormat::D32_Float;
    const EFormat LightProbeFormat     = EFormat::R16G16B16A16_Float;
    const UInt32  MaxPointLights       = 256;
    const UInt32  MaxDirectionalLights = 256;
    const UInt32  MaxPointLightShadows = 8;
    const UInt16  ShadowMapWidth       = 4096;
    const UInt16  ShadowMapHeight      = 4096;
    const UInt16  PointLightShadowSize = 1024;

    TSharedRef<ConstantBuffer> PointLightBuffer;
    TSharedRef<ConstantBuffer> ShadowPointLightBuffer;
    TSharedRef<ConstantBuffer> DirectionalLightBuffer;

    TSharedRef<TextureCubeArray> PointLightShadowMaps;
    TArray<DepthStencilViewCube> PointLightShadowMapDSVs;

    TSharedRef<Texture2D>           DirLightShadowMaps;
    TSharedRef<TextureCube>         IrradianceMap;
    TSharedRef<UnorderedAccessView> IrradianceMapUAV;

    TSharedRef<TextureCube>                 SpecularIrradianceMap;
    TArray<TSharedRef<UnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<UnorderedAccessView*>            WeakSpecularIrradianceMapUAVs;
};