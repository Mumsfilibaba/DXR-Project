#pragma once
#include "DeferredRenderer.h"
#include "ShadowMapRenderer.h"
#include "ScreenSpaceOcclusionRenderer.h"
#include "LightProbeRenderer.h"
#include "SkyboxRenderPass.h"
#include "ForwardRenderer.h"
#include "RayTracer.h"
#include "DebugRenderer.h"
#include "TemporalAA.h"
#include "RendererScene.h"
#include "Core/Time/Stopwatch.h"
#include "Core/Threading/AsyncTask.h"
#include "Application/Events.h"
#include "Application/ApplicationEventHandler.h"
#include "Engine/Scene/Actors/Actor.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Camera.h"
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

struct FCameraBuffer
{
    FMatrix4 PrevViewProjection;
    
    FMatrix4 ViewProjection;
    FMatrix4 ViewProjectionInv;

    FMatrix4 ViewProjectionUnjittered;
    FMatrix4 ViewProjectionInvUnjittered;

    FMatrix4 View;
    FMatrix4 ViewInv;

    FMatrix4 Projection;
    FMatrix4 ProjectionInv;

    FVector3 Position;
    float    NearPlane = 0.0f;

    FVector3 Forward;
    float    FarPlane = 0.0f;

    FVector3 Right;
    float    AspectRatio = 0.0f;

    FVector2 Jitter;
    FVector2 PrevJitter;

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

    void Tick(FRendererScene* Scene);

    void OnWindowResize(const FWindowEvent& Event);

    void PerformFXAA(FRHICommandList& InCmdList);
    void PerformBackBufferBlit(FRHICommandList& InCmdList);

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
    bool InitAA();
    bool InitShadingImage();

    // RenderPasses and Resources
    FFrameResources               Resources;
    FLightSetup                   LightSetup;

    FCameraBuffer                 CameraBuffer;
    FHaltonState                  HaltonState;

    FDeferredRenderer             DeferredRenderer;
    FShadowMapRenderer            ShadowMapRenderer;
    FScreenSpaceOcclusionRenderer SSAORenderer;
    FLightProbeRenderer           LightProbeRenderer;
    FSkyboxRenderPass             SkyboxRenderPass;
    FForwardRenderer              ForwardRenderer;
    FRayTracer                    RayTracer;
    FDebugRenderer                DebugRenderer;
    FTemporalAA                   TemporalAA;

    // RHI
    FRHICommandList               CommandList;
    FRHICommandStatistics         FrameStatistics;

    FRHIQueryRef                  TimestampQueries;
    
    FRHITextureRef                ShadingImage;
    FRHIComputePipelineStateRef   ShadingRatePipeline;
    FRHIComputeShaderRef          ShadingRateShader;

    FRHIGraphicsPipelineStateRef  PostPSO;
    FRHIPixelShaderRef            PostShader;

    FRHIGraphicsPipelineStateRef  FXAAPSO;
    FRHIPixelShaderRef            FXAAShader;
    FRHIGraphicsPipelineStateRef  FXAADebugPSO;
    FRHIPixelShaderRef            FXAADebugShader;

    // Event handling and Widgets
    TOptional<FWindowEvent>              ResizeEvent;
    TSharedPtr<FRenderTargetDebugWindow> TextureDebugger;
    TSharedPtr<FRendererInfoWindow>      InfoWindow;
    TSharedPtr<FGPUProfilerWindow>       GPUProfilerWindow;
    TSharedPtr<FRendererEventHandler>    EventHandler;

    static FSceneRenderer* GInstance;
};
