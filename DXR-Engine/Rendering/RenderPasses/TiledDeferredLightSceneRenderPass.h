#pragma once
#include "SceneRenderPass.h"

class TiledDeferredLightSceneRenderPass final : public SceneRenderPass
{
public:
	TiledDeferredLightSceneRenderPass()  = default;
	~TiledDeferredLightSceneRenderPass() = default;

	Bool Init(SharedRenderPassResources& FrameResources);

	virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
	TSharedRef<ComputePipelineState> PipelineState;
};