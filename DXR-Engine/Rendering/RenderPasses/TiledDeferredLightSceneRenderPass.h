#pragma once
#include "SceneRenderPass.h"

class TiledDeferredLightSceneRenderPass final : public SceneRenderPass
{
public:
	TiledDeferredLightSceneRenderPass()  = default;
	~TiledDeferredLightSceneRenderPass() = default;

	virtual Bool Init(SharedRenderPassResources& FrameResources) override;

	virtual Bool ResizeResources(SharedRenderPassResources& FrameResources) override;

	virtual void Render(
		CommandList& CmdList, 
		SharedRenderPassResources& FrameResources,
		const Scene& Scene) override;

private:
	Bool CreateRenderTarget(SharedRenderPassResources& FrameResources);

	TSharedRef<ComputePipelineState> PipelineState;
};