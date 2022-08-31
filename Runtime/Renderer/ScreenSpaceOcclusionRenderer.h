#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FScreenSpaceOcclusionRenderer

class RENDERER_API FScreenSpaceOcclusionRenderer
{
public:
    FScreenSpaceOcclusionRenderer() = default;
    ~FScreenSpaceOcclusionRenderer() = default;

    bool Init(FFrameResources& FrameResources);
    void Release();

    void Render(FRHICommandList& CommandList, FFrameResources& FrameResources);

    bool ResizeResources(FFrameResources& FrameResources);

private:
    bool CreateRenderTarget(FFrameResources& FrameResources);

    FRHIComputePipelineStateRef PipelineState;
    FRHIComputeShaderRef        SSAOShader;
    
    FRHIComputePipelineStateRef BlurHorizontalPSO;
    FRHIComputeShaderRef        BlurHorizontalShader;
    
    FRHIComputePipelineStateRef BlurVerticalPSO;
    FRHIComputeShaderRef        BlurVerticalShader;

    FRHIGenericBufferRef        SSAOSamples;
    FRHIShaderResourceViewRef   SSAOSamplesSRV;
    FRHITexture2DRef            SSAONoiseTex;
};