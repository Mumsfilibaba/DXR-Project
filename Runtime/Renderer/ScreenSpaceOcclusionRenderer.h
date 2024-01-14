#pragma once
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Scene/Scene.h"

class RENDERER_API FScreenSpaceOcclusionRenderer
{
public:
    bool Initialize(FFrameResources& FrameResources);

    void Release();

    void Render(FRHICommandList& CommandList, FFrameResources& FrameResources);

    bool ResizeResources(FRHICommandList& CommandList, FFrameResources& FrameResources, uint32 Width, uint32 Height);

private:
    bool CreateRenderTarget(FFrameResources& FrameResources, uint32 Width, uint32 Height);

    FRHIComputePipelineStateRef PipelineState;
    FRHIComputeShaderRef        SSAOShader;
    
    FRHIComputePipelineStateRef BlurHorizontalPSO;
    FRHIComputeShaderRef        BlurHorizontalShader;
    
    FRHIComputePipelineStateRef BlurVerticalPSO;
    FRHIComputeShaderRef        BlurVerticalShader;
};