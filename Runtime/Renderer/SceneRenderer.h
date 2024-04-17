#pragma once
#include "DeferredRendering.h"
#include "ShadowRendering.h"
#include "ScreenSpaceOcclusionRendering.h"
#include "LightProbeRenderer.h"
#include "SkyboxRenderPass.h"
#include "ForwardPass.h"
#include "RayTracer.h"
#include "DebugRendering.h"
#include "TemporalAA.h"
#include "Scene.h"
#include "PostProcessing.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Threading/AsyncTask.h"
#include "Application/Events.h"
#include "Application/ApplicationEventHandler.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/World.h"
#include "Engine/World/Camera.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Debug/TextureDebugger.h"
#include "Debug/RendererInfoWindow.h"
#include "Debug/GPUProfilerWindow.h"

class FViewport;
class FSceneRenderer;

struct FCameraHLSL
{
    // 0-64
    FMatrix4 PrevViewProjection;
    // 64-192
    FMatrix4 ViewProjection;
    FMatrix4 ViewProjectionInv;
    // 192-320
    FMatrix4 ViewProjectionUnjittered;
    FMatrix4 ViewProjectionInvUnjittered;
    // 320-448
    FMatrix4 View;
    FMatrix4 ViewInv;
    // 448-576
    FMatrix4 Projection;
    FMatrix4 ProjectionInv;
    // 576-592
    FVector3 Position;
    float    NearPlane = 0.0f;
    // 592-608
    FVector3 Forward;
    float    FarPlane = 0.0f;
    // 608-624
    FVector3 Right;
    float    AspectRatio = 0.0f;
    // 624-640
    FVector2 Jitter;
    FVector2 PrevJitter;
    // 640-656
    float    ViewportWidth  = 0.0f;
    float    ViewportHeight = 0.0f;
    float    Padding0       = 0.0f;
    float    Padding1       = 0.0f;
};

class FRendererEventHandler : public FApplicationEventHandler
{
public:
    FRendererEventHandler(FSceneRenderer* InRenderer)
        : Renderer(InRenderer)
    {
    }

    virtual FResponse OnWindowResized(const FWindowEvent& WindowEvent) override final;

private:
    FSceneRenderer* Renderer;
};

class FSceneRenderer
{
public:
    FSceneRenderer();
    ~FSceneRenderer();

    bool Initialize();
    bool InitializeRenderPasses();

    void Tick(FScene* Scene);

    void ResizeResources(const FWindowEvent& Event);

    void AddDebugTexture(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        TextureDebugger->AddTextureForDebugging(ImageView, Image, BeforeState, AfterState);
    }

    TSharedPtr<FRenderTargetDebugWindow> GetTextureDebugger() const
    {
        return TextureDebugger;
    }

    const FRHICommandStatistics& GetStatistics() const
    {
        return FrameStatistics;
    }

private:
    bool InitShadingImage();

    // RenderPasses and Resources
    FFrameResources             Resources;

    FCameraHLSL                 CameraBuffer;
    FHaltonState                HaltonState;

    FDepthPrePass*              DepthPrePass;
    FDeferredBasePass*          BasePass;
    FDepthReducePass*           DepthReducePass;
    FTiledLightPass*            TiledLightPass;
    FPointLightRenderPass*      PointLightRenderPass;
    FCascadeGenerationPass*     CascadeGenerationPass;
    FCascadedShadowsRenderPass* CascadedShadowsRenderPass;
    FShadowMaskRenderPass*      ShadowMaskRenderPass;
    FScreenSpaceOcclusionPass*  ScreenSpaceOcclusionPass;
    FSkyboxRenderPass*          SkyboxRenderPass;
    FTemporalAA*                TemporalAA;
    FForwardPass*               ForwardPass;
    FFXAAPass*                  FXAAPass;
    FTonemapPass*               TonemapPass;

    FLightProbeRenderer         LightProbeRenderer;
    FRayTracer                  RayTracer;
    FDebugRenderer              DebugRenderer;

    // RHI
    FRHIQueryRef                TimestampQueries;
    FRHICommandList             CommandList;
    FRHICommandStatistics       FrameStatistics;

    FRHITextureRef              ShadingImage;
    FRHIComputePipelineStateRef ShadingRatePipeline;
    FRHIComputeShaderRef        ShadingRateShader;

    // Event handling
    TOptional<FWindowEvent>     ResizeEvent;

    // Widgets
    TSharedPtr<FRenderTargetDebugWindow> TextureDebugger;
    TSharedPtr<FRendererInfoWindow>      InfoWindow;
    TSharedPtr<FGPUProfilerWindow>       GPUProfilerWindow;
    TSharedPtr<FRendererEventHandler>    EventHandler;
};
