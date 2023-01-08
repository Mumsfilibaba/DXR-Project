#pragma once
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Scene/Scene.h"

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
};