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
#include "Core/Time/Stopwatch.h"
#include "Core/Threading/AsyncTask.h"
#include "Engine/Scene/Actors/Actor.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "RHI/RHIInterface.h"
#include "RHI/RHICommandList.h"
#include "Debug/TextureDebugger.h"
#include "Debug/RendererInfoWindow.h"
#include "Debug/GPUProfilerWindow.h"

class FViewport;

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
    float    Padding0 = 0.0f;
    float    Padding1 = 0.0f;
};

class RENDERER_API FRenderer
{
    FRenderer();
    ~FRenderer();

public:
    static bool Initialize();
    static void Release();

    static FRenderer& Get();

    void Tick(const FScene& Scene);

    void PerformFrustumCullingAndSort(const FScene& Scene);
    void PerformFXAA(FRHICommandList& InCmdList);
    void PerformBackBufferBlit(FRHICommandList& InCmdList);

    FORCEINLINE TSharedRef<FRenderTargetDebugWindow> GetTextureDebugger() const
    {
        return TextureDebugger;
    }

    FORCEINLINE const FRHICommandStatistics& GetStatistics() const
    {
        return FrameStatistics;
    }

private:
    void OnWindowResize(FViewport* Viewport, const FWindowResizeEvent& Event);

    bool Create();
    
    bool InitAA();
    bool InitShadingImage();

    NOINLINE void FrustumCullingAndSortingInternal(
        const FCamera* Camera,
        const TPair<uint32, uint32>& DrawCommands,
        TArray<uint32>& OutDeferredDrawCommands,
        TArray<uint32>& OutForwardDrawCommands);

    TSharedRef<FRenderTargetDebugWindow> TextureDebugger;
    TSharedRef<FRendererInfoWindow>      InfoWindow;
    TSharedRef<FGPUProfilerWindow>       GPUProfilerWindow;

    FRHICommandList CommandList;

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

    FRHITextureRef               ShadingImage;
    FRHIComputePipelineStateRef  ShadingRatePipeline;
    FRHIComputeShaderRef         ShadingRateShader;

    FRHIGraphicsPipelineStateRef PostPSO;
    FRHIPixelShaderRef           PostShader;

    FRHIGraphicsPipelineStateRef FXAAPSO;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    FRHIPixelShaderRef           FXAADebugShader;

    FRHITimestampQueryRef TimestampQueries;

    FRHICommandStatistics FrameStatistics;

    static FRenderer* GInstance;
};


inline void AddDebugTexture(
    const FRHIShaderResourceViewRef& ImageView,
    const FRHITextureRef& Image,
    EResourceAccess BeforeState,
    EResourceAccess AfterState)
{
    FRenderer::Get().GetTextureDebugger()->AddTextureForDebugging(ImageView, Image, BeforeState, AfterState);
}
