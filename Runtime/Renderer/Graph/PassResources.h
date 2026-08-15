#pragma once
#include "Renderer/Graph/FrameResources.h"

class FRenderGraphPassResources;
class FRenderGraphTexture;
class FRenderGraphBuffer;
struct FSceneRenderGraphContext;

struct FPassResources
{
    const FFrameResources* FrameResources = nullptr;

    FRHITexture* GBufferAlbedo           = nullptr;
    FRHITexture* GBufferNormal           = nullptr;
    FRHITexture* GBufferMaterial         = nullptr;
    FRHITexture* GBufferVelocity         = nullptr;
    FRHITexture* GBufferDepth            = nullptr;
    FRHITexture* SSAOBuffer              = nullptr;
    FRHITexture* SceneTarget             = nullptr;
    FRHITexture* TonemappedTarget        = nullptr;
    FRHITexture* DirectionalShadowMask   = nullptr;
    FRHITexture* CascadeIndexBuffer      = nullptr;
    FRHITexture* ReducedDepthBuffer0     = nullptr;
    FRHITexture* ReducedDepthBuffer1     = nullptr;
    FRHITexture* BackBuffer              = nullptr;
    FRHITexture* ShadowCascades          = nullptr;
    FRHITexture* PointLightShadowMaps    = nullptr;
    FRHITexture* RayTracingOutput        = nullptr;
#if EDITOR_BUILD
    FRHITexture* EditorNoJitterDepth     = nullptr;
    FRHITexture* EditorObjectID_NoJitter = nullptr;
#endif

    int32 RenderWidth  = 0;
    int32 RenderHeight = 0;
};

struct PassResourceSync
{
    static FPassResources Create(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context);
    static void SyncToFrameResources(const FPassResources& PassResources, FFrameResources& FrameResources);
    static void SyncGraphViews(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context, FFrameResources& FrameResources);
    static void SyncCascadedShadowViews(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context, FFrameResources& FrameResources);
    static void SyncPointLightShadowViews(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context, FFrameResources& FrameResources);
};
