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

    TSharedRef<ComputePipelineState> PipelineState;
    TSharedRef<ComputeShader>        SSAOShader;
    TSharedRef<ComputePipelineState> BlurHorizontalPSO;
    TSharedRef<ComputeShader>        BlurHorizontalShader;
    TSharedRef<ComputePipelineState> BlurVerticalPSO;
    TSharedRef<ComputeShader>        BlurVerticalShader;
    TSharedRef<StructuredBuffer>     SSAOSamples;
    TSharedRef<ShaderResourceView>   SSAOSamplesSRV;
    TSharedRef<Texture2D>            SSAONoiseTex;
};