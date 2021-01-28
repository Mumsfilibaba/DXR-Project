#pragma once
#include "SceneRenderPass.h"

class ForwardSceneRenderPass final : public SceneRenderPass
{
public:
    ForwardSceneRenderPass()  = default;
    ~ForwardSceneRenderPass() = default;

    virtual Bool Init(SharedRenderPassResources& FrameResources) override;

    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResources
        const Scene& Scene) override;

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
};