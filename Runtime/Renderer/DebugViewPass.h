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

private:
    FRHIGraphicsPipelineStateRef DebugPSO_Linear;
    FRHIGraphicsPipelineStateRef DebugPSO_BackBuffer;
    FRHIVertexShaderRef          DebugVertexShader;
    FRHIPixelShaderRef           DebugPixelShader;
};
