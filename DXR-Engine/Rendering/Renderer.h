#pragma once
#include "Windows/WindowsWindow.h"

#include "Time/Clock.h"

#include "Application/InputCodes.h"
#include "Application/Events/EventHandler.h"

#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Resources/MeshFactory.h"

#include "DeferredRenderer.h"
#include "ShadowMapRenderer.h"
#include "ScreenSpaceOcclusionRenderer.h"
#include "LightProbeRenderer.h"
#include "SkyboxRenderPass.h"
#include "ForwardRenderer.h"
#include "RayTracer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/CommandList.h"
#include "RenderLayer/Viewport.h"

#include "DebugUI.h"

class Renderer
{
public:
    Renderer()  = default;
    ~Renderer() = default;

    bool Init();
    void Release();

    void PerformFrustumCulling(const Scene& Scene);
    void PerformFXAA(CommandList& InCmdList);
    void PerformBackBufferBlit(CommandList& InCmdList);

    void PerformAABBDebugPass(CommandList& InCmdList);

    void RenderDebugInterface();

    void Tick(const Scene& Scene);

private:
    bool InitBoundingBoxDebugPass();
    bool InitAA();
    bool InitShadingImage();

    void ResizeResources(uint32 Width, uint32 Height);

    CommandList CmdList;

    DeferredRenderer             DeferredRenderer;
    ShadowMapRenderer            ShadowMapRenderer;
    ScreenSpaceOcclusionRenderer SSAORenderer;
    LightProbeRenderer           LightProbeRenderer;
    SkyboxRenderPass             SkyboxRenderPass;
    ForwardRenderer              ForwardRenderer;
    RayTracer                    RayTracer;

    FrameResources Resources;
    LightSetup     LightSetup;

    TRef<Texture2D>            ShadingImage;
    TRef<ComputePipelineState> ShadingRatePipeline;
    TRef<ComputeShader>        ShadingRateShader;

    TRef<VertexBuffer> AABBVertexBuffer;
    TRef<IndexBuffer>  AABBIndexBuffer;
    TRef<GraphicsPipelineState> AABBDebugPipelineState;
    TRef<VertexShader>          AABBVertexShader;
    TRef<PixelShader>           AABBPixelShader;

    TRef<GraphicsPipelineState> PostPSO;
    TRef<PixelShader>           PostShader;
    TRef<GraphicsPipelineState> FXAAPSO;
    TRef<PixelShader>           FXAAShader;
    TRef<GraphicsPipelineState> FXAADebugPSO;
    TRef<PixelShader>           FXAADebugShader;

    TRef<GPUProfiler> GPUProfiler;

    uint32 LastFrameNumDrawCalls     = 0;
    uint32 LastFrameNumDispatchCalls = 0;
    uint32 LastFrameNumCommands      = 0;
};