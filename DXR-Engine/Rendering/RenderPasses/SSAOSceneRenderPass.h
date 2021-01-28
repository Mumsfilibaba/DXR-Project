#pragma once
#include "SceneRenderPass.h"

class SSAOSceneRenderPass final : public SceneRenderPass
{
public:
    SSAOSceneRenderPass()  = default;
    ~SSAOSceneRenderPass() = default;

    virtual Bool Init(SharedRenderPassResources& FrameResources) override;

    virtual Bool ResizeResources(SharedRenderPassResources& FrameResources) override;
    
    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResources,
        const Scene& Scene) override;

private:
    Bool CreateRenderTarget(SharedRenderPassResources& FrameResources);

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