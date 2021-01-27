#pragma once
#include "SceneRenderPass.h"

class DeferredSceneRenderPass final : public SceneRenderPass
{
public:
	DeferredSceneRenderPass() = default;
	DeferredSceneRenderPass() = default;

	Bool Init(SharedRenderPassResources& FrameResources);

	virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override final;

private:
	TSharedRef<GraphicsPipelineState> PipelineState;
};