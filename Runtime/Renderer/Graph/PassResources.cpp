#include "Renderer/Graph/PassResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "RendererCore/RenderGraph/RenderGraphPass.h"

template<typename ResourceType>
static void AssignResourceRef(TSharedRef<ResourceType>& Dest, ResourceType* Source)
{
    if (!Source || Dest.Get() == Source)
    {
        return;
    }

    Dest = MakeSharedRef<ResourceType>(Source);
}

static FRHITexture* ResolveTexture(const FRenderGraphPassResources& GraphResources, FRenderGraphTexture* Texture)
{
    return Texture ? GraphResources.TryGet(Texture) : nullptr;
}

void PassResourceSync::SyncGraphViews(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context, FFrameResources& FrameResources)
{
    for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        if (Context.ShadowCascadeDSVs[CascadeIndex])
        {
            if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.ShadowCascadeDSVs[CascadeIndex]))
            {
                AssignResourceRef(FrameResources.ShadowCascadePerCascadeDSVs[CascadeIndex], DepthStencilView);
            }
        }
    }

    if (Context.ShadowCascadesCombinedDSV)
    {
        if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.ShadowCascadesCombinedDSV))
        {
            AssignResourceRef(FrameResources.ShadowCascadesCombinedDSV, DepthStencilView);
        }
    }

    if (Context.CascadeMatrixBufferUAV)
    {
        if (FRHIUnorderedAccessView* UnorderedAccessView = GraphResources.TryGet(Context.CascadeMatrixBufferUAV))
        {
            AssignResourceRef(FrameResources.CascadeMatrixBufferUAV, UnorderedAccessView);
        }
    }

    if (Context.CascadeMatrixBufferSRV)
    {
        if (FRHIShaderResourceView* ShaderResourceView = GraphResources.TryGet(Context.CascadeMatrixBufferSRV))
        {
            AssignResourceRef(FrameResources.CascadeMatrixBufferSRV, ShaderResourceView);
        }
    }

    if (Context.CascadeSplitsBufferUAV)
    {
        if (FRHIUnorderedAccessView* UnorderedAccessView = GraphResources.TryGet(Context.CascadeSplitsBufferUAV))
        {
            AssignResourceRef(FrameResources.CascadeSplitsBufferUAV, UnorderedAccessView);
        }
    }

    if (Context.CascadeSplitsBufferSRV)
    {
        if (FRHIShaderResourceView* ShaderResourceView = GraphResources.TryGet(Context.CascadeSplitsBufferSRV))
        {
            AssignResourceRef(FrameResources.CascadeSplitsBufferSRV, ShaderResourceView);
        }
    }

    if (Context.PointLightShadowMapDSVs.Size() > 0)
    {
        FrameResources.PointLightShadowMapDSVs.Resize(Context.PointLightShadowMapDSVs.Size());

        for (int32 LightIndex = 0; LightIndex < Context.PointLightShadowMapDSVs.Size(); ++LightIndex)
        {
            if (Context.PointLightShadowMapDSVs[LightIndex])
            {
                if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.PointLightShadowMapDSVs[LightIndex]))
                {
                    AssignResourceRef(FrameResources.PointLightShadowMapDSVs[LightIndex], DepthStencilView);
                }
            }
        }
    }

    if (Context.PointLightShadowMapFaceDSVs.Size() > 0)
    {
        FrameResources.PointLightShadowMapFaceDSVs.Resize(Context.PointLightShadowMapFaceDSVs.Size());

        for (int32 FaceIndex = 0; FaceIndex < Context.PointLightShadowMapFaceDSVs.Size(); ++FaceIndex)
        {
            if (Context.PointLightShadowMapFaceDSVs[FaceIndex])
            {
                if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.PointLightShadowMapFaceDSVs[FaceIndex]))
                {
                    AssignResourceRef(FrameResources.PointLightShadowMapFaceDSVs[FaceIndex], DepthStencilView);
                }
            }
        }
    }
}

void PassResourceSync::SyncCascadedShadowViews(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context, FFrameResources& FrameResources)
{
    for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        if (Context.ShadowCascadeDSVs[CascadeIndex])
        {
            if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.ShadowCascadeDSVs[CascadeIndex]))
            {
                AssignResourceRef(FrameResources.ShadowCascadePerCascadeDSVs[CascadeIndex], DepthStencilView);
            }
        }
    }

    if (Context.ShadowCascadesCombinedDSV)
    {
        if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.ShadowCascadesCombinedDSV))
        {
            AssignResourceRef(FrameResources.ShadowCascadesCombinedDSV, DepthStencilView);
        }
    }
}

void PassResourceSync::SyncPointLightShadowViews(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context, FFrameResources& FrameResources)
{
    if (Context.PointLightShadowMapDSVs.Size() > 0)
    {
        FrameResources.PointLightShadowMapDSVs.Resize(Context.PointLightShadowMapDSVs.Size());

        for (int32 LightIndex = 0; LightIndex < Context.PointLightShadowMapDSVs.Size(); ++LightIndex)
        {
            if (Context.PointLightShadowMapDSVs[LightIndex])
            {
                if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.PointLightShadowMapDSVs[LightIndex]))
                {
                    AssignResourceRef(FrameResources.PointLightShadowMapDSVs[LightIndex], DepthStencilView);
                }
            }
        }
    }

    if (Context.PointLightShadowMapFaceDSVs.Size() > 0)
    {
        FrameResources.PointLightShadowMapFaceDSVs.Resize(Context.PointLightShadowMapFaceDSVs.Size());

        for (int32 FaceIndex = 0; FaceIndex < Context.PointLightShadowMapFaceDSVs.Size(); ++FaceIndex)
        {
            if (Context.PointLightShadowMapFaceDSVs[FaceIndex])
            {
                if (FRHIDepthStencilView* DepthStencilView = GraphResources.TryGet(Context.PointLightShadowMapFaceDSVs[FaceIndex]))
                {
                    AssignResourceRef(FrameResources.PointLightShadowMapFaceDSVs[FaceIndex], DepthStencilView);
                }
            }
        }
    }
}

FPassResources PassResourceSync::Create(const FRenderGraphPassResources& GraphResources, const FSceneRenderGraphContext& Context)
{
    if (Context.FrameResources)
    {
        SyncGraphViews(GraphResources, Context, *Context.FrameResources);
    }

    FPassResources Result;
    Result.FrameResources          = Context.FrameResources;
    Result.RenderWidth             = Context.FrameResources ? int32(Context.FrameResources->CurrentRenderWidth) : 0;
    Result.RenderHeight            = Context.FrameResources ? int32(Context.FrameResources->CurrentRenderHeight) : 0;
    Result.GBufferAlbedo           = ResolveTexture(GraphResources, Context.GBufferAlbedo);
    Result.GBufferNormal           = ResolveTexture(GraphResources, Context.GBufferNormal);
    Result.GBufferMaterial         = ResolveTexture(GraphResources, Context.GBufferMaterial);
    Result.GBufferVelocity         = ResolveTexture(GraphResources, Context.GBufferVelocity);
    Result.GBufferDepth            = ResolveTexture(GraphResources, Context.GBufferDepth);
    Result.SSAOBuffer              = ResolveTexture(GraphResources, Context.SSAOBuffer);
    Result.SceneTarget             = ResolveTexture(GraphResources, Context.SceneTarget);
    Result.TonemappedTarget        = ResolveTexture(GraphResources, Context.TonemappedTarget);
    Result.DirectionalShadowMask   = ResolveTexture(GraphResources, Context.DirectionalShadowMask);
    Result.CascadeIndexBuffer      = ResolveTexture(GraphResources, Context.CascadeIndexBuffer);
    Result.ReducedDepthBuffer0     = ResolveTexture(GraphResources, Context.ReducedDepthBuffer0);
    Result.ReducedDepthBuffer1     = ResolveTexture(GraphResources, Context.ReducedDepthBuffer1);
    Result.BackBuffer              = ResolveTexture(GraphResources, Context.BackBuffer);
    Result.ShadowCascades          = ResolveTexture(GraphResources, Context.ShadowCascades);
    Result.PointLightShadowMaps    = ResolveTexture(GraphResources, Context.PointLightShadowMaps);
    Result.RayTracingOutput        = ResolveTexture(GraphResources, Context.RayTracingOutput);
#if EDITOR_BUILD
    Result.EditorNoJitterDepth     = ResolveTexture(GraphResources, Context.EditorNoJitterDepth);
    Result.EditorObjectID_NoJitter = ResolveTexture(GraphResources, Context.EditorObjectID_NoJitter);
#endif

    return Result;
}

void PassResourceSync::SyncToFrameResources(const FPassResources& PassResources, FFrameResources& FrameResources)
{
    AssignResourceRef(FrameResources.GBuffer[EGBufferIndex::Albedo], PassResources.GBufferAlbedo);
    AssignResourceRef(FrameResources.GBuffer[EGBufferIndex::Normal], PassResources.GBufferNormal);
    AssignResourceRef(FrameResources.GBuffer[EGBufferIndex::Material], PassResources.GBufferMaterial);
    AssignResourceRef(FrameResources.GBuffer[EGBufferIndex::Velocity], PassResources.GBufferVelocity);
    AssignResourceRef(FrameResources.GBuffer[EGBufferIndex::Depth], PassResources.GBufferDepth);
    AssignResourceRef(FrameResources.SSAOBuffer, PassResources.SSAOBuffer);
    AssignResourceRef(FrameResources.SceneTarget, PassResources.SceneTarget);
    AssignResourceRef(FrameResources.TonemappedTarget, PassResources.TonemappedTarget);
    AssignResourceRef(FrameResources.DirectionalShadowMask, PassResources.DirectionalShadowMask);
    AssignResourceRef(FrameResources.CascadeIndexBuffer, PassResources.CascadeIndexBuffer);
    AssignResourceRef(FrameResources.ReducedDepthBuffer[0], PassResources.ReducedDepthBuffer0);
    AssignResourceRef(FrameResources.ReducedDepthBuffer[1], PassResources.ReducedDepthBuffer1);
#if EDITOR_BUILD
    AssignResourceRef(FrameResources.EditorNoJitterDepth, PassResources.EditorNoJitterDepth);
    AssignResourceRef(FrameResources.EditorObjectID_NoJitter, PassResources.EditorObjectID_NoJitter);
#endif
}
