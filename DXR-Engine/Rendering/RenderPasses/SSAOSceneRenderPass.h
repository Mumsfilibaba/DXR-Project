#pragma once
#include "SceneRenderPass.h"

class SSAOSceneRenderPass final : public SceneRenderPass
{
public:
    SSAOSceneRenderPass()  = default;
    ~SSAOSceneRenderPass() = default;

    Bool Init();

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResources) override;

private:
    TSharedPtr<ComputePipelineState> SSAOPSO;
    TSharedPtr<ComputePipelineState> SSAOBlurHorizontal;
    TSharedPtr<ComputePipelineState> SSAOBlurVertical;
    TSharedRef<StructuredBuffer>     SSAOSamples;
    TSharedRef<ShaderResourceView>   SSAOSamplesSRV;
    TSharedRef<Texture2D>            SSAOBuffer;
    TSharedRef<ShaderResourceView>   SSAOBufferSRV;
    TSharedRef<UnorderedAccessView>  SSAOBufferUAV;
    TSharedRef<Texture2D>            SSAONoiseTex;
    TSharedRef<ShaderResourceView>   SSAONoiseSRV;
};