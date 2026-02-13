#pragma once
#include "Core/Time/ElapsedTime.h"
#include "Core/Threading/AsyncTask.h"
#include "Core/Containers/Queue.h"
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
#include "Renderer/PostProcessing.h"
#include "Renderer/DebugViewPass.h"
#if EDITOR_BUILD
#include "Renderer/SelectionOutlinePass.h"
#endif
#include "Renderer/Scene/Scene.h"
#include "Renderer/RendererUI/TextureDebugWidget.h"
#include "Renderer/RendererUI/RendererInfoWidget.h"
#include "Renderer/RendererUI/GPUProfilerWidget.h"

class FViewportWidget;
class FSceneRenderer;

#if EDITOR_BUILD
class FEditorNoJitterDepthPass;
class FEditorSelectionIDPass;
#endif

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

    // 448-576
    FMatrix4 ProjectionUnjittered;
    FMatrix4 ProjectionInvUnjittered;

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

struct FSwapChainResizeInfo
{
    FSwapChainResizeInfo() = default;

    FSwapChainResizeInfo(FRHISwapChainRef InSwapChain, uint32 InWidth, uint32 InHeight)
        : SwapChain(InSwapChain)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    FRHISwapChainRef SwapChain = nullptr;
    uint32 Width  = 0;
    uint32 Height = 0;
};

class FSceneRenderer
{
public:
    FSceneRenderer();
    ~FSceneRenderer();

    bool Initialize();
    bool InitializeRenderPasses();
    
    void BeginFrame();
    
    void Tick(FScene* Scene);
    
    void RenderSceneView(const FSceneRenderView& SceneRenderView);
    void RenderUI();

    void EndFrame(); 
 
    void RequestEditorObjectPick(FScene* Scene, uint32 PixelX, uint32 PixelY); 
    bool PollEditorObjectPickResult(FScene* Scene, uint32& OutObjectID); 
 
    void ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 InWidth, uint32 InHeight); 
    void PrepareSwapChain(FRHISwapChainRef SwapChain); 
    void PresentSwapChain(FRHISwapChainRef SwapChain); 

    void ResizeResources(uint32 InWidth, uint32 InHeight);

    void AddDebugTexture(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image)
    {
        TextureDebugger->AddTextureForDebugging(ImageView, Image);
    }

    TSharedPtr<FTextureDebugWidget> GetTextureDebugger() const
    {
        return TextureDebugger;
    }

    uint32 GetRenderWidth() const
    {
        return Resources.CurrentRenderWidth;
    }
    
    uint32 GetRenderHeight() const
    {
        return Resources.CurrentRenderHeight;
    }
    
    const FFrameCounterState& GetFrameCounter() const
    {
        return FrameCounter;
    }

    FRHITexture* GetSelectionMaskTexture() const
    {
#if EDITOR_BUILD
        return SelectionOutlinePass ? SelectionOutlinePass->GetSelectionMask() : nullptr;
#else
        return nullptr;
#endif
    }

    FRHITexture* GetSelectionDilatedMaskTexture() const
    {
#if EDITOR_BUILD
        return SelectionOutlinePass ? SelectionOutlinePass->GetDilatedMask() : nullptr;
#else
        return nullptr;
#endif
    }

    FRHITexture* GetSelectionRingTexture() const
    {
#if EDITOR_BUILD
        return SelectionOutlinePass ? SelectionOutlinePass->GetRingMask() : nullptr;
#else
        return nullptr;
#endif
    }

private: 
    bool InitShadingImage(); 
#if EDITOR_BUILD
    void ProcessEditorObjectPickRequests(FRHICommandList& InCommandList, FFrameResources& InResources, FScene* CurrentScene);
#endif
 
    // RenderPasses and Resources 
    FFrameResources             Resources; 
    FFrameCounterState          FrameCounter;

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
#if EDITOR_BUILD
    FSelectionOutlinePass*      SelectionOutlinePass;
    FEditorNoJitterDepthPass*   EditorNoJitterDepthPass;
    FEditorSelectionIDPass*     EditorSelectionIDPass;
#endif
    FForwardPass*               ForwardPass;
    FFXAAPass*                  FXAAPass;
    FTonemapPass*               TonemapPass;
    FDebugViewPass*             DebugViewPass;
#if EDITOR_BUILD
    FFinalCompositePass*        FinalCompositePass;
#endif
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

    // SwapChains that should be presented at the end of the frame
    TArray<FRHISwapChainRef>     SwapChainsToPrepare;
    TArray<FRHISwapChainRef>     SwapChainsToPresent;
    TArray<FSwapChainResizeInfo> SwapChainsToResize;

#if EDITOR_BUILD
    struct FEditorObjectPickRequest
    {
        FScene* Scene = nullptr;
        uint32  PixelX = 0;
        uint32  PixelY = 0;
    };

    struct FEditorObjectPickInFlight
    {
        FScene*         Scene = nullptr;

        FRHIBufferRef   ReadbackBuffer;
        FRHIGpuFenceRef Fence;

        uint32          SampleRadius = 0;
        uint32          PixelX       = 0;
        uint32          PixelY       = 0;
        uint32          TexWidth     = 0;
        uint32          TexHeight    = 0;

        // Normal window (around PixelX/PixelY).
        uint32          NormalBaseOffset     = 0;
        uint32          NormalRowStrideBytes = 0;
        uint32          NormalWidth          = 0;
        uint32          NormalHeight         = 0;
        uint32          NormalCenterX        = 0;
        uint32          NormalCenterY        = 0;

        // Optional flipped-Y window.
        uint32          bHasFlippedWindow : 1 = 0;
        uint32          FlippedBaseOffset     = 0;
        uint32          FlippedRowStrideBytes = 0;
        uint32          FlippedWidth          = 0;
        uint32          FlippedHeight         = 0;
        uint32          FlippedCenterX        = 0;
        uint32          FlippedCenterY        = 0;
    };

    static constexpr uint32 MaxInFlightObjectPicks = 4;
    TQueue<FEditorObjectPickRequest, EQueueType::MPSC> PendingObjectPicks;
    TArray<FEditorObjectPickInFlight>                  InFlightObjectPicks;
#endif

    // Widgets
    TSharedPtr<FTextureDebugWidget> TextureDebugger;
    TSharedPtr<FRendererInfoWidget> InfoWindow;
    TSharedPtr<FGPUProfilerWidget>  GPUProfilerWindow;
};
