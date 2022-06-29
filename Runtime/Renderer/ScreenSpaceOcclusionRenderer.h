#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CScreenSpaceOcclusionRenderer

class RENDERER_API CScreenSpaceOcclusionRenderer
{
public:
    CScreenSpaceOcclusionRenderer() = default;
    ~CScreenSpaceOcclusionRenderer() = default;

    bool Init(SFrameResources& FrameResources);
    void Release();

    void Render(CRHICommandList& CmdList, SFrameResources& FrameResources);

    bool ResizeResources(SFrameResources& FrameResources);

private:
    bool CreateRenderTarget(SFrameResources& FrameResources);

    TSharedRef<FRHIComputePipelineState> PipelineState;
    TSharedRef<FRHIComputeShader>        SSAOShader;
    TSharedRef<FRHIComputePipelineState> BlurHorizontalPSO;
    TSharedRef<FRHIComputeShader>        BlurHorizontalShader;
    TSharedRef<FRHIComputePipelineState> BlurVerticalPSO;
    TSharedRef<FRHIComputeShader>        BlurVerticalShader;
    TSharedRef<FRHIGenericBuffer>     SSAOSamples;
    TSharedRef<FRHIShaderResourceView>   SSAOSamplesSRV;
    TSharedRef<FRHITexture2D>            SSAONoiseTex;
};