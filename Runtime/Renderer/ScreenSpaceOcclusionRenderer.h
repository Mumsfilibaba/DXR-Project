#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

class RENDERER_API CScreenSpaceOcclusionRenderer
{
public:
    CScreenSpaceOcclusionRenderer() = default;
    ~CScreenSpaceOcclusionRenderer() = default;

    bool Init( SFrameResources& FrameResources );
    void Release();

    void Render( CRHICommandList& CmdList, SFrameResources& FrameResources );

    bool ResizeResources( SFrameResources& FrameResources );

private:
    bool CreateRenderTarget( SFrameResources& FrameResources );

    TSharedRef<CRHIComputePipelineState> PipelineState;
    TSharedRef<CRHIComputeShader>        SSAOShader;
    TSharedRef<CRHIComputePipelineState> BlurHorizontalPSO;
    TSharedRef<CRHIComputeShader>        BlurHorizontalShader;
    TSharedRef<CRHIComputePipelineState> BlurVerticalPSO;
    TSharedRef<CRHIComputeShader>        BlurVerticalShader;
    TSharedRef<CRHIStructuredBuffer>     SSAOSamples;
    TSharedRef<CRHIShaderResourceView>   SSAOSamplesSRV;
    TSharedRef<CRHITexture2D>            SSAONoiseTex;
};