#pragma once
#include "SceneRenderPass.h"

class ForwardSceneRenderPass final : public SceneRenderPass
{
public:
    ForwardSceneRenderPass()  = default;
    ~ForwardSceneRenderPass() = default;

    Bool Init();

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
};