#pragma once
#include "SceneRenderPass.h"

class DebugSceneRenderPass final : public SceneRenderPass
{
public:
    DebugSceneRenderPass()  = default;
    ~DebugSceneRenderPass() = default;

    virtual Bool Init(SharedRenderPassResources& FrameResources) override;

    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResources, 
        const Scene& Scene) override;

private:
    TSharedRef<VertexBuffer> AABBVertexBuffer;
    TSharedRef<IndexBuffer>  AABBIndexBuffer;
    TSharedRef<GraphicsPipelineState> PipelineState;
};