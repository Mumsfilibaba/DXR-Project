#pragma once
#include "SceneRenderPass.h"

struct DirectionalLightProperties
{
    XMFLOAT3   Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float      ShadowBias    = 0.005f;
    XMFLOAT3   Direction     = XMFLOAT3(0.0f, -1.0f, 0.0f);
    Float      MaxShadowBias = 0.05f;
    XMFLOAT4X4 LightMatrix;
};

class DirectionalLightShadowSceneRenderPass final : public SceneRenderPass
{
public:
    DirectionalLightShadowSceneRenderPass()  = default;
    ~DirectionalLightShadowSceneRenderPass() = default;

    virtual Bool Init(SharedRenderPassResources& FrameResources) override;

    virtual Bool ResizeResources(SharedRenderPassResources& FrameResources) override;
    
    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResources,
        const Scene& Scene) override;

private:
    Bool CreateShadowMaps(SharedRenderPassResources& FrameResources);

    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<ConstantBuffer>        PerShadowMapBuffer;

    Bool   UpdateDirLight = true;
    UInt64 DirLightFrame  = 0;

    // TODO: Fix VSM
    //TSharedRef<GraphicsPipelineState> VSMShadowMapPSO; 
};