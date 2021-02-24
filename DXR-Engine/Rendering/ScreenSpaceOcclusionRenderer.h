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

    void Render(CommandList& CmdList, FrameResources& FrameResources);

    Bool ResizeResources(FrameResources& FrameResources);

private:
    Bool CreateRenderTarget(FrameResources& FrameResources);

    TRef<ComputePipelineState> PipelineState;
    TRef<ComputeShader>        SSAOShader;
    TRef<ComputePipelineState> BlurHorizontalPSO;
    TRef<ComputeShader>        BlurHorizontalShader;
    TRef<ComputePipelineState> BlurVerticalPSO;
    TRef<ComputeShader>        BlurVerticalShader;
    TRef<StructuredBuffer>     SSAOSamples;
    TRef<ShaderResourceView>   SSAOSamplesSRV;
    TRef<Texture2D>            SSAONoiseTex;
};