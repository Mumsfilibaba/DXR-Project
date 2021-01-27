#pragma once
#include "SceneRenderPass.h"

class SSAOSceneRenderPass final : public SceneRenderPass
{
public:
    SSAOSceneRenderPass()  = default;
    ~SSAOSceneRenderPass() = default;

    Bool Init(SharedRenderPassResources& FrameResources);

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    Bool InitRenderTarget(SharedRenderPassResources& FrameResources);

    TSharedPtr<ComputePipelineState> PipelineState;
    TSharedPtr<ComputePipelineState> BlurHorizontalPSO;
    TSharedPtr<ComputePipelineState> BlurVerticalPSO;
    TSharedRef<StructuredBuffer>     SSAOSamples;
    TSharedRef<ShaderResourceView>   SSAOSamplesSRV;
    TSharedRef<Texture2D>            SSAOBuffer;
    TSharedRef<ShaderResourceView>   SSAOBufferSRV;
    TSharedRef<UnorderedAccessView>  SSAOBufferUAV;
    TSharedRef<Texture2D>            SSAONoiseTex;
    TSharedRef<ShaderResourceView>   SSAONoiseSRV;
};