#pragma once
#include "SceneRenderPass.h"

class LightProbeSceneRenderPass final : public SceneRenderPass
{
public:
    LightProbeSceneRenderPass()  = default;
    ~LightProbeSceneRenderPass() = default;

    Bool Init(SharedRenderPassResources& FrameResources);

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    TSharedRef<ComputePipelineState> IrradianceGenPSO;
    TSharedRef<ComputePipelineState> SpecularIrradianceGenPSO;
};