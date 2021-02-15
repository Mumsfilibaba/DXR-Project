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

    Bool Init();
    void Release();

    void PerformFrustumCulling(const Scene& Scene);
    void PerformFXAA(CommandList& InCmdList);
    void PerformBackBufferBlit(CommandList& InCmdList);

    void PerformAABBDebugPass(CommandList& InCmdList);

    void RenderDebugInterface();

    void Tick(const Scene& Scene);

private:
    Bool InitBoundingBoxDebugPass();
    Bool InitAA();
    Bool InitShadingImage();

    void ResizeResources(UInt32 Width, UInt32 Height);

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

    TRef<VertexBuffer> AABBVertexBuffer;
    TRef<IndexBuffer>  AABBIndexBuffer;
    TRef<GraphicsPipelineState> AABBDebugPipelineState;

    TRef<GraphicsPipelineState> PostPSO;
    TRef<GraphicsPipelineState> FXAAPSO;
    TRef<GraphicsPipelineState> FXAADebugPSO;

    UInt32 LastFrameNumDrawCalls     = 0;
    UInt32 LastFrameNumDispatchCalls = 0;
    UInt32 LastFrameNumCommands      = 0;
};