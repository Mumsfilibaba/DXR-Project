#pragma once
#include "FrameResources.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class ScreenSpaceOcclusionRenderer
{
public:
	ScreenSpaceOcclusionRenderer()  = default;
	~ScreenSpaceOcclusionRenderer() = default;

    Bool Init(FrameResources& FrameResources);
    void Release();

    Bool ResizeResources(FrameResources& FrameResources);

    void Render(
        CommandList& CmdList,
        const FrameResources& FrameResources);

private:
    Bool CreateRenderTarget(FrameResources& FrameResources);

    TSharedPtr<ComputePipelineState> PipelineState;
    TSharedPtr<ComputePipelineState> BlurHorizontalPSO;
    TSharedPtr<ComputePipelineState> BlurVerticalPSO;
    TSharedRef<StructuredBuffer>     SSAOSamples;
    TSharedRef<ShaderResourceView>   SSAOSamplesSRV;
    TSharedRef<Texture2D>            SSAONoiseTex;
    TSharedRef<ShaderResourceView>   SSAONoiseSRV;
};