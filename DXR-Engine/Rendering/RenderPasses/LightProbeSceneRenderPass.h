#pragma once
#include "SceneRenderPass.h"

class LightProbeSceneRenderPass final : public SceneRenderPass
{
public:
    LightProbeSceneRenderPass()  = default;
    ~LightProbeSceneRenderPass() = default;

    virtual Bool Init(SharedRenderPassResources& FrameResources) override;

    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResources,
        const Scene& Scene) override;

private:
    TSharedRef<ComputePipelineState> IrradianceGenPSO;
    TSharedRef<ComputePipelineState> SpecularIrradianceGenPSO;
};