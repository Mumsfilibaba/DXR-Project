#pragma once
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"
#include "RendererCore/Interfaces/IRendererModule.h"

class FDebugViewPass : public FRenderPass
{
public:
    FDebugViewPass(FSceneRenderer* InRenderer);
    virtual ~FDebugViewPass();

    bool Initialize(const FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, FSceneRenderView::EDebugView DebugView);
    void ExecuteOverlay(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, FSceneRenderView::EDebugView DebugView, int32 X, int32 Y, int32 Width, int32 Height);

private:
    void ExecuteInternal(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, 
        FSceneRenderView::EDebugView DebugView, int32 X, int32 Y, int32 Width, int32 Height, bool bClearTarget, bool bPreferTonemapped);

    FRHIGraphicsPipelineStateRef DebugPSO_Linear;
    FRHIGraphicsPipelineStateRef DebugPSO_BackBuffer;
    FRHIVertexShaderRef          DebugVertexShader;
    FRHIPixelShaderRef           DebugPixelShader;
};
