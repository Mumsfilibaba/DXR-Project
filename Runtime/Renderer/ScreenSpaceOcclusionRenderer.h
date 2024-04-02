#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Scene/Scene.h"

class FScreenSpaceOcclusionRenderer : public FRenderPass
{
public:
    FScreenSpaceOcclusionRenderer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

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