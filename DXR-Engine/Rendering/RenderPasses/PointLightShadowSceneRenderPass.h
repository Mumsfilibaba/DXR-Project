#pragma once
#include "SceneRenderPass.h"

class PointLightShadowSceneRenderPass final : public SceneRenderPass
{
public:
    PointLightShadowSceneRenderPass()  = default;
    ~PointLightShadowSceneRenderPass() = default;

    Bool Init(SharedRenderPassResources& FrameResources);

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<ConstantBuffer>        PerShadowMapBuffer;

    Bool   UpdatePointLight = true;
    UInt64 PointLightFrame  = 0;
};