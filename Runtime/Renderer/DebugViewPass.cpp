#include "Renderer/DebugViewPass.h"

FDebugViewPass::FDebugViewPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FDebugViewPass::~FDebugViewPass() = default;

bool FDebugViewPass::Initialize(const FFrameResources& FrameResources)
{
    (void)FrameResources;
    return true;
}

void FDebugViewPass::Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, FSceneRenderView::EDebugView DebugView)
{
    (void)CommandList;
    (void)SceneRenderView;
    (void)FrameResources;
    (void)DebugView;
}
