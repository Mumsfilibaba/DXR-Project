#pragma once
#include "Core/Time/Stopwatch.h"
#include "Core/Threading/AsyncTask.h"
#include "Application/Events.h"
#include "Application/InputHandler.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/World.h"
#include "Engine/World/Camera.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Renderer/DeferredRendering.h"
#include "Renderer/ShadowRendering.h"
#include "Renderer/ScreenSpaceOcclusionRendering.h"
#include "Renderer/LightProbeRenderer.h"
#include "Renderer/SkyboxRenderPass.h"
#include "Renderer/ForwardPass.h"
#include "Renderer/RayTracer.h"
#include "Renderer/DebugRendering.h"
#include "Renderer/TemporalAA.h"
#include "Renderer/Scene.h"
#include "Renderer/PostProcessing.h"
#include "Renderer/Widgets/TextureDebugWidget.h"
#include "Renderer/Widgets/RendererInfoWidget.h"
#include "Renderer/Widgets/GPUProfilerWidget.h"
#include "Renderer/Widgets/RendererSettingsWidget.h"

class FViewportWidget;
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

class FFrameCounterState
{
public:
    FFrameCounterState(const uint32 InMaxFrames = TNumericLimits<uint32>::Max())
        : MaxFrames(InMaxFrames)
        , FrameIndex(0)
    {
    }

    void NextFrame()
    {
        FrameIndex = (FrameIndex + 1) % MaxFrames;
    }

    uint32 GetFrameIndex() const
    {
        return FrameIndex;
    }

private:
    const uint32 MaxFrames;
    uint32       FrameIndex;
};

class FSceneRenderer
{
public:
    FSceneRenderer();
    ~FSceneRenderer();

    bool Initialize();
    bool InitializeRenderPasses();
    
    void Tick(FScene* Scene);

    void ResizeResources(uint32 InWidth, uint32 InHeight);

    void AddDebugTexture(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image, EResourceAccess BeforeState, EResourceAccess AfterState)
    {
        TextureDebugger->AddTextureForDebugging(ImageView, Image, BeforeState, AfterState);
    }

    TSharedPtr<FTextureDebugWidget> GetTextureDebugger() const
    {
        return TextureDebugger;
    }

    uint32 GetRenderWidth() const
    {
        return Resources.CurrentWidth;
    }
    
    uint32 GetRenderHeight() const
    {
        return Resources.CurrentHeight;
    }
    
    const FFrameCounterState& GetFrameCounter() const
    {
        return FrameCounter;
    }

private:
    bool InitShadingImage();

    // RenderPasses and Resources
    FFrameResources             Resources;
    FFrameCounterState          FrameCounter;

    FCameraHLSL                 CameraBuffer;
    FHaltonState                HaltonState;

    FDepthPrePass*              DepthPrePass;
    FDeferredBasePass*          BasePass;
    FOcclusionPass*             OcclusionPass;
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
    FLightProbeRenderer*        LightProbeRenderer;
    FDebugRenderer*             DebugRenderer;

    FRayTracer                  RayTracer;

    // RHI
    FGenericEvent*              LastFrameFinishedEvent;
    FRHIQueryRef                TimestampQueries;
    FRHICommandList             CommandList;

    FRHITextureRef              ShadingImage;
    FRHIComputePipelineStateRef ShadingRatePipeline;
    FRHIComputeShaderRef        ShadingRateShader;

    // Widgets
    TSharedPtr<FTextureDebugWidget>     TextureDebugger;
    TSharedPtr<FRendererInfoWidget>     InfoWindow;
    TSharedPtr<FGPUProfilerWidget>      GPUProfilerWindow;
    TSharedPtr<FRendererSettingsWidget> SettingsWindow;
};
