#pragma once
#include "FrameResources.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class ScreenSpaceOcclusionRenderer
{
public:
    ScreenSpaceOcclusionRenderer() = default;
    ~ScreenSpaceOcclusionRenderer() = default;

    bool Init( FrameResources& FrameResources );
    void Release();

    void Render( CommandList& CmdList, FrameResources& FrameResources );

    bool ResizeResources( FrameResources& FrameResources );

private:
    bool CreateRenderTarget( FrameResources& FrameResources );

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