#pragma once
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FRenderGraphBuilder;

class FDebugViewPass : public FRenderPass
{
public:
    FDebugViewPass(FSceneRenderer* InRenderer);
    virtual ~FDebugViewPass();

    bool Initialize(const FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);
    void AddRenderGraphOverlayPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

    void Record(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources,
        FSceneRenderView::EDebugView DebugView);
    void PreparePipelineStateForFormat(EFormat OutputFormat);

private:
    void RecordInternal(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources,
        FSceneRenderView::EDebugView DebugView, int32 X, int32 Y, int32 Width, int32 Height, bool bClearTarget, bool bPreferTonemapped);
    void RecordOverlay(FRHICommandList& CommandList, const FPassResources& PassResources, const FSceneRenderView& SceneRenderView,
        FSceneRenderView::EDebugView DebugView, int32 X, int32 Y, int32 Width, int32 Height);

    FRHIGraphicsPipelineStateRef DebugPSO;
    EFormat                      DebugPSOFormat = EFormat::Unknown;

    FRHIVertexShaderRef          DebugVertexShader;
    FRHIPixelShaderRef           DebugPixelShader;
    FRHIDepthStencilStateRef     DebugDepthStencilState;
    FRHIRasterizerStateRef       DebugRasterizerState;
    FRHIBlendStateRef            DebugBlendState;
};
