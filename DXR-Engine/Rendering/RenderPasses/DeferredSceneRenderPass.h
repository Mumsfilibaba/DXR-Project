#pragma once
#include "SceneRenderPass.h"

class DeferredSceneRenderPass final : public SceneRenderPass
{
public:
    DeferredSceneRenderPass() = default;
    DeferredSceneRenderPass() = default;

    virtual Bool Init(SharedRenderPassResources& FrameResources) override;
    
    virtual Bool ResizeResources(SharedRenderPassResources& FrameResources) override;

    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResources,
        const Scene& Scene) override;

private:
    Bool CreateGBuffer(SharedRenderPassResources& FrameResources);

    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<GraphicsPipelineState> PrePassPipelineState;
};