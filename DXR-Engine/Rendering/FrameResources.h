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
    const EFormat SSAOBufferFormat   = EFormat::R16G16B16A16_Float;
    const EFormat FinalTargetFormat  = EFormat::R16G16B16A16_Float;
    const EFormat RTOutputFormat     = EFormat::R16G16B16A16_Float;
    const EFormat RenderTargetFormat = EFormat::R8G8B8A8_Unorm;
    const EFormat AlbedoFormat       = EFormat::R8G8B8A8_Unorm;
    const EFormat MaterialFormat     = EFormat::R8G8B8A8_Unorm;
    const EFormat NormalFormat       = EFormat::R10G10B10A2_Unorm;
    const EFormat ViewNormalFormat   = EFormat::R10G10B10A2_Unorm;

    Texture2D* BackBuffer = nullptr;

    TRef<ConstantBuffer> CameraBuffer;
    TRef<ConstantBuffer> TransformBuffer;

    TRef<SamplerState> DirectionalShadowSampler;
    TRef<SamplerState> PointShadowSampler;
    TRef<SamplerState> IrradianceSampler;

    TRef<TextureCube> Skybox;

    TRef<Texture2D>    IntegrationLUT;
    TRef<SamplerState> IntegrationLUTSampler;

    TRef<Texture2D>    ReflectionTexture;
    TRef<Texture2D>    SSAOBuffer;
    TRef<Texture2D>    FinalTarget;
    TRef<Texture2D>    GBuffer[5];
    TRef<SamplerState> GBufferSampler;
    TRef<SamplerState> FXAASampler;

    TRef<InputLayoutState> StdInputLayout;

    TRef<Texture2D>       RTOutput;
    TRef<RayTracingScene> RTScene;
    TArray<RayTracingGeometryInstance> RTGeometryInstances;
    TArray<RayTracingShaderResources>  RTHitGroupResources;

    TArray<MeshDrawCommand> DeferredVisibleCommands;
    TArray<MeshDrawCommand> ForwardVisibleCommands;

    TArray<ImGuiImage> DebugTextures;

    TRef<Viewport> MainWindowViewport;
};

