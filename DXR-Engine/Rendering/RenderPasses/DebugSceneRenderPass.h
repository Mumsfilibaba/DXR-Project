#pragma once
#include "SceneRenderPass.h"

class DebugSceneRenderPass final : public SceneRenderPass
{
public:
    DebugSceneRenderPass()  = default;
    ~DebugSceneRenderPass() = default;

    Bool Init();

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    TSharedRef<VertexBuffer> AABBVertexBuffer;
    TSharedRef<IndexBuffer>  AABBIndexBuffer;
    TSharedRef<GraphicsPipelineState> PipelineState;
};