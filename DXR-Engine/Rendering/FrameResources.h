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

    TSharedRef<RayTracingScene>        RTScene;
    TArray<RayTracingGeometryInstance> RTGeometryInstances;

    TArray<MeshDrawCommand> DeferredVisibleCommands;
    TArray<MeshDrawCommand> ForwardVisibleCommands;

    TArray<ImGuiImage> DebugTextures;

    TSharedRef<Viewport> MainWindowViewport;
};

