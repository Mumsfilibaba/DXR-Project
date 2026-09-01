#pragma once
#include "Core/Time/ElapsedTime.h"
#include "Core/Platform/PlatformEvent.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Containers/Queue.h"
#include "Application/Events.h"
#include "Application/InputHandler.h"
#include "ApplicationRenderer/ApplicationRenderer.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/World.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Renderer/Passes/DepthPrePass.h"
#include "Renderer/Passes/DeferredBasePass.h"
#include "Renderer/Passes/TiledLightPass.h"
#include "Renderer/Passes/DepthReducePass.h"
#include "Renderer/Passes/PointLightRenderPass.h"
#include "Renderer/Passes/CascadeGenerationPass.h"
#include "Renderer/Passes/CascadedShadowsRenderPass.h"
#include "Renderer/Passes/ShadowMaskRenderPass.h"
#include "Renderer/Passes/ScreenSpaceOcclusionRendering.h"
#include "Renderer/LightProbeRenderer.h"
#include "Renderer/Passes/SkyboxRenderPass.h"
#include "Renderer/Passes/ForwardPass.h"
#include "Renderer/Passes/RayTracing/RayTracingSceneBuilder.h"
#include "Renderer/Passes/RayTracing/RayTracingReflectionsPass.h"
#include "Renderer/Passes/RayTracing/ReflectionDenoisePass.h"
#include "Renderer/Passes/RayTracing/RayTracingPrimaryDebugPass.h"
#include "Renderer/DebugRendering.h"
#include "Renderer/Passes/DebugViewPass.h"
#include "Renderer/Passes/TemporalAntiAliasing.h"
#include "Renderer/Passes/TonemapPass.h"
#include "Renderer/Passes/FXAAPass.h"
#if EDITOR_BUILD
    #include "Renderer/Passes/Editor/FinalCompositePass.h"
    #include "Renderer/Passes/Editor/SelectionOutlinePass.h"
    #include "Renderer/Passes/Editor/EditorNoJitterDepthPass.h"
    #include "Renderer/Passes/Editor/EditorSelectionIDPass.h"
#endif
#include "Renderer/Scene/Scene.h"

// Variable rate shading is scaffolding: the shading rate image is built but nothing binds it yet
#define SUPPORT_VARIABLE_RATE_SHADING (0)

class FViewport;
class FSceneRenderer;

struct FCameraHLSL
{
    // 0-64
    Matrix4 PrevViewProjection;

    // 64-192
    Matrix4 ViewProjection;
    Matrix4 ViewProjectionInv;

    // 192-320
    Matrix4 ViewProjectionUnjittered;
    Matrix4 ViewProjectionInvUnjittered;

    // 320-448
    Matrix4 View;
    Matrix4 ViewInv;

    // 448-576
    Matrix4 Projection;
    Matrix4 ProjectionInv;

    // 448-576
    Matrix4 ProjectionUnjittered;
    Matrix4 ProjectionInvUnjittered;

    // 576-592
    Vector3 Position;
    float   NearPlane = 0.0f;

    // 592-608
    Vector3 Forward;
    float   FarPlane = 0.0f;

    // 608-624
    Vector3 Right;
    float   AspectRatio = 0.0f;

    // 624-640
    Vector2 ProjectionJitter;
    Vector2 PrevProjectionJitter;

    // 640-656
    float   ViewportWidth  = 0.0f;
    float   ViewportHeight = 0.0f;
    Vector2 ImageJitter;

    // 656-672
    Vector3 PrevPosition;
    float   Padding2       = 0.0f;
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

    FSwapChainResizeInfo(FRHISwapChainRef InSwapChain, uint32 InWidth, uint32 InHeight, EFormat InFormat = EFormat::Unknown, EColorSpace InColorSpace = EColorSpace::Unknown)
        : SwapChain(InSwapChain)
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
        , ColorSpace(InColorSpace)
    {
    }

    NODISCARD bool HasPendingChange() const
    {
        return Width != 0u || Height != 0u || Format != EFormat::Unknown || ColorSpace != EColorSpace::Unknown;
    }

    FRHISwapChainRef SwapChain  = nullptr;
    uint32           Width      = 0;                    // 0 == keep current
    uint32           Height     = 0;                    // 0 == keep current
    EFormat          Format     = EFormat::Unknown;     // Unknown == keep current
    EColorSpace      ColorSpace = EColorSpace::Unknown; // Unknown == keep current
};

class FSceneRenderer
{
public:
    FSceneRenderer();
    ~FSceneRenderer();

    bool Initialize();
    bool InitializeRenderPasses();

    // Records BeginFrame + scene apply/cull + RenderSceneView into the scene commandlist and dispatches it.
    void RenderThread_RenderSceneFrame(const FSceneRenderPacket& Packet);
    void RenderThread_PrepareResources(const FSceneRenderView& SceneRenderView, FScene* Scene);

    // Records ImGui draw data into the UI command list.
    void RecordUI();

    // Records the Application element library on top of the ImGui pass.
    void RecordApplicationUI();

    // Dispatches the UI command list plus present for the frame described by Packet.
    void SubmitUIAndPresent(const FSceneRenderPacket& Packet);

    void RequestEditorObjectPick(FScene* Scene, uint32 PixelX, uint32 PixelY, uint64 RequestId);
    bool PollEditorObjectPickResult(FScene* Scene, FEditorPickResult& OutResult);

    void RequestEditorObjectPickRect(FScene* Scene, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY);
    bool PollEditorObjectPickRectResult(FScene* Scene, TArray<uint32>& OutObjectIDs);
 
    void ResizeSwapChain(FRHISwapChainRef SwapChain, uint32 InWidth, uint32 InHeight, EFormat InFormat = EFormat::Unknown, EColorSpace InColorSpace = EColorSpace::Unknown); 
    void ResizeResources(uint32 InWidth, uint32 InHeight);

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
#if SUPPORT_VARIABLE_RATE_SHADING
    bool InitShadingImage();
#endif

    bool CreateRayTracingResources(uint32 Width, uint32 Height);

    void RenderThread_PrepareCameraData(const FSceneRenderView& SceneRenderView, FScene* Scene);
    void RenderThread_BeginSceneCommandList(const FSceneRenderPacket& Packet);
    void RenderThread_RenderSceneView(const FSceneRenderView& SceneRenderView, const TArray<uint32>& SelectedObjectIDs);
#if EDITOR_BUILD
    void RenderThread_ProcessEditorObjectPickRequests(FRHICommandList& InCommandList, FFrameResources& InResources, FScene* CurrentScene);
#endif

    void BuildAndExecuteSceneGraph(const FSceneRenderView& SceneRenderView, FScene* Scene, const TArray<uint32>& SelectedObjectIDs);

    // RenderPasses and Resources 
    FFrameResources                  Resources; 
    FFrameCounterState               FrameCounter;
    FCameraHLSL                      CameraBuffer;
    FHaltonState                     HaltonState;
    FRHISamplePositionsDesc          FrameSamplePositions;
    FRHISamplePositionsDesc          PrevFrameSamplePositions;
    bool                             bUseHardwareJitter = false;
    FDepthPrePass*                   DepthPrePass;
    FDeferredBasePass*               BasePass;
    FDepthReducePass*                DepthReducePass;
    FTiledLightPass*                 TiledLightPass;
    FPointLightRenderPass*           PointLightRenderPass;
    FCascadeGenerationPass*          CascadeGenerationPass;
    FCascadedShadowsRenderPass*      CascadedShadowsRenderPass;
    FShadowMaskRenderPass*           ShadowMaskRenderPass;
    FScreenSpaceOcclusionPass*       ScreenSpaceOcclusionPass;
    FSkyboxRenderPass*               SkyboxRenderPass;
    FTemporalAntiAliasing*           TemporalAntiAliasing;
#if EDITOR_BUILD
    FSelectionOutlinePass*           SelectionOutlinePass;
    FEditorNoJitterDepthPass*        EditorNoJitterDepthPass;
    FEditorSelectionIDPass*          EditorSelectionIDPass;
#endif
    FForwardPass*                    ForwardPass;
    FFXAAPass*                       FXAAPass;
    FTonemapPass*                    TonemapPass;
#if EDITOR_BUILD
    FFinalCompositePass*             FinalCompositePass;
#endif
    FLightProbeRenderer*             LightProbeRenderer;
    FDebugRenderer*                  DebugRenderer;
    FDebugViewPass*                  DebugViewPass;
    FRayTracingSceneBuilder*         RayTracingSceneBuilder;
    FRayTracingReflectionsPass*      RayTracingReflectionsPass;
    FReflectionDenoisePass*          ReflectionDenoisePass;
    FRayTracingPrimaryDebugPass*     RayTracingPrimaryDebugPass;
    bool                             bRayTracingWasActive = false; // tracks the RT active->inactive edge for BLAS teardown
    IPlatformEvent*                  LastFrameFinishedEvent;
    TSharedPtr<FApplicationRenderer> ApplicationRenderer;
    FRHIQueryRef                     TimestampQueries;
    FRHICommandList                  CommandList;
    FRHICommandList                  UICommandList;
#if SUPPORT_VARIABLE_RATE_SHADING
    FRHITextureRef                   ShadingImage;
    FRHIComputePipelineStateRef      ShadingRatePipeline;
    FRHIComputeShaderRef             ShadingRateShader;
#endif
    TArray<FSwapChainResizeInfo>     SwapChainsToResize;
    FCriticalSection                 SwapChainsToResizeCS;

#if EDITOR_BUILD
    struct FEditorObjectPickRequest
    {
        FScene* Scene = nullptr;

        // For a rect pick these are the top-left corner and MaxX/MaxY the bottom-right, for a point pick they are the pixel
        uint32  PixelX    = 0;
        uint32  PixelY    = 0;
        uint32  MaxX      = 0;
        uint32  MaxY      = 0;
        bool    bIsRect   = false;
        uint64  RequestId = 0;
    };

    struct FEditorObjectPickInFlight
    {
        FScene*       Scene = nullptr;
        uint64        RequestId = 0;

        FRHIFenceRef  Fence;
        FRHIBufferRef ReadbackBuffer;

        uint32        SampleRadius = 0;
        uint32        PixelX       = 0;
        uint32        PixelY       = 0;
        uint32        TexWidth     = 0;
        uint32        TexHeight    = 0;

        // A rect pick collects every ID in the window instead of the one nearest its center
        uint32        bIsRectPick : 1 = 0;

        // Normal window (around PixelX/PixelY).
        uint32        NormalBaseOffset     = 0;
        uint32        NormalRowStrideBytes = 0;
        uint32        NormalWidth          = 0;
        uint32        NormalHeight         = 0;
        uint32        NormalCenterX        = 0;
        uint32        NormalCenterY        = 0;

        // Optional flipped-Y window.
        uint32        bHasFlippedWindow : 1 = 0;
        uint32        FlippedBaseOffset     = 0;
        uint32        FlippedRowStrideBytes = 0;
        uint32        FlippedWidth          = 0;
        uint32        FlippedHeight         = 0;
        uint32        FlippedCenterX        = 0;
        uint32        FlippedCenterY        = 0;

        // Depth window, always the same rectangle as the normal ObjectID window.
        uint32        bHasDepthWindow : 1 = 0;
        uint32        DepthBaseOffset     = 0;
        uint32        DepthRowStrideBytes = 0;
    };

    static constexpr uint32 MaxInFlightObjectPicks = 4;

    TQueue<FEditorObjectPickRequest, EQueueType::MPSC> PendingObjectPicks;
    TArray<FEditorObjectPickInFlight>                  InFlightObjectPicks;
    FCriticalSection                                   ObjectPickStateCS;
#endif
};
