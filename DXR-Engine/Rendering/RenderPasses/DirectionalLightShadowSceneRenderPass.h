#pragma once
#include "SceneRenderPass.h"

class DirectionalLightShadowSceneRenderPass final : public SceneRenderPass
{
public:
    DirectionalLightShadowSceneRenderPass()  = default;
    ~DirectionalLightShadowSceneRenderPass() = default;

    Bool Init(SharedRenderPassResources& FrameResources);

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<ConstantBuffer>        PerShadowMapBuffer;

    Bool   UpdateDirLight = true;
    UInt64 DirLightFrame  = 0;

    // TODO: Fix VSM
    //TSharedRef<GraphicsPipelineState> VSMShadowMapPSO; 
};