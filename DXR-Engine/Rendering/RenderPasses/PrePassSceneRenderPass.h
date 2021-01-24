#pragma once
#include "SceneRenderPass.h"

class PrePassSceneRenderPass final : public SceneRenderPass
{
public:
    PrePassSceneRenderPass()  = default;
    ~PrePassSceneRenderPass() = default;

    Bool Init();

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
};