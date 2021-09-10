#pragma once

#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

#include "Assets/MeshFactory.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"

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

#include "Core/Time/Timer.h"
#include "Core/Threading/TaskManager.h"

class Renderer
{
public:
    bool Init();

    void Tick( const Scene& Scene );

    void Release();

    void PerformFrustumCulling( const Scene& Scene );
    void PerformFXAA( CommandList& InCmdList );
    void PerformBackBufferBlit( CommandList& InCmdList );

    void PerformAABBDebugPass( CommandList& InCmdList );

    void RenderDebugInterface();

private:
    void OnWindowResize( const WindowResizeEvent& Event );

    bool InitBoundingBoxDebugPass();
    bool InitAA();
    bool InitShadingImage();

    void ResizeResources( uint32 Width, uint32 Height );

    CommandList PreShadowsCmdList;
    CommandList PointShadowCmdList;
    CommandList DirShadowCmdList;
    CommandList PrepareGBufferCmdList;
    CommandList PrePassCmdList;
    CommandList ShadingRateCmdList;
    CommandList RayTracingCmdList;
    CommandList BasePassCmdList;
    CommandList MainCmdList;

    Task PointShadowTask;
    Task DirShadowTask;
    Task PrePassTask;
    Task BasePassTask;
    Task RayTracingTask;

    DeferredRenderer             DeferredRenderer;
    ShadowMapRenderer            ShadowMapRenderer;
    ScreenSpaceOcclusionRenderer SSAORenderer;
    LightProbeRenderer           LightProbeRenderer;
    SkyboxRenderPass             SkyboxRenderPass;
    ForwardRenderer              ForwardRenderer;
    RayTracer                    RayTracer;

    FrameResources Resources;
    LightSetup     LightSetup;

    TSharedRef<Texture2D>            ShadingImage;
    TSharedRef<ComputePipelineState> ShadingRatePipeline;
    TSharedRef<ComputeShader>        ShadingRateShader;

    TSharedRef<VertexBuffer> AABBVertexBuffer;
    TSharedRef<IndexBuffer>  AABBIndexBuffer;
    TSharedRef<GraphicsPipelineState> AABBDebugPipelineState;
    TSharedRef<VertexShader>          AABBVertexShader;
    TSharedRef<PixelShader>           AABBPixelShader;

    TSharedRef<GraphicsPipelineState> PostPSO;
    TSharedRef<PixelShader>           PostShader;
    TSharedRef<GraphicsPipelineState> FXAAPSO;
    TSharedRef<PixelShader>           FXAAShader;
    TSharedRef<GraphicsPipelineState> FXAADebugPSO;
    TSharedRef<PixelShader>           FXAADebugShader;

    TSharedRef<GPUProfiler> GPUProfiler;

    uint32 LastFrameNumDrawCalls = 0;
    uint32 LastFrameNumDispatchCalls = 0;
    uint32 LastFrameNumCommands = 0;
};

extern Renderer GRenderer;