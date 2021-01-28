#pragma once
#include "SceneRenderPass.h"

struct PointLightProperties
{
    XMFLOAT3 Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float    ShadowBias    = 0.005f;
    XMFLOAT3 Position      = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Float    FarPlane      = 10.0f;
    Float    MaxShadowBias = 0.05f;
    Float    Radius        = 5.0f;

    Float Padding0;
    Float Padding1;
};

class PointLightShadowSceneRenderPass final : public SceneRenderPass
{
public:
    PointLightShadowSceneRenderPass()  = default;
    ~PointLightShadowSceneRenderPass() = default;

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

    Bool   UpdatePointLight = true;
    UInt64 PointLightFrame  = 0;
};