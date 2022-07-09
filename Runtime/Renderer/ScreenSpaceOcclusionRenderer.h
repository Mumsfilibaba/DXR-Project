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

    void Render(FRHICommandList& CmdList, SFrameResources& FrameResources);

    bool ResizeResources(SFrameResources& FrameResources);

private:
    bool CreateRenderTarget(SFrameResources& FrameResources);

    FRHIComputePipelineStateRef PipelineState;
    FRHIComputeShaderRef        SSAOShader;
    FRHIComputePipelineStateRef BlurHorizontalPSO;
    FRHIComputeShaderRef        BlurHorizontalShader;
    FRHIComputePipelineStateRef BlurVerticalPSO;
    FRHIComputeShaderRef        BlurVerticalShader;
    TSharedRef<FRHIGenericBuffer>     SSAOSamples;
    TSharedRef<FRHIShaderResourceView>   SSAOSamplesSRV;
    FRHITexture2DRef            SSAONoiseTex;
};