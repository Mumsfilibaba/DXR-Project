#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "Engine/Scene/Scene.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CDeferredRenderer

class RENDERER_API CDeferredRenderer
{
public:
    CDeferredRenderer() = default;
    ~CDeferredRenderer() = default;

    bool Init(SFrameResources& FrameResources);

    void Release();

    void RenderPrePass(CRHICommandList& CmdList, SFrameResources& FrameResources, const CScene& Scene);
    void RenderBasePass(CRHICommandList& CmdList, const SFrameResources& FrameResources);
    void RenderDeferredTiledLightPass(CRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup);

    bool ResizeResources(SFrameResources& FrameResources);

private:
    bool CreateGBuffer(SFrameResources& FrameResources);

    CRHIGraphicsPipelineStateRef PipelineState;
    CRHIVertexShaderRef          BaseVertexShader;
    CRHIPixelShaderRef           BasePixelShader;

    CRHIGraphicsPipelineStateRef PrePassPipelineState;
    CRHIVertexShaderRef          PrePassVertexShader;

    TSharedRef<CRHIComputePipelineState>  TiledLightPassPSO;
    TSharedRef<CRHIComputeShader>         TiledLightShader;
    TSharedRef<CRHIComputePipelineState>  TiledLightPassPSODebug;
    TSharedRef<CRHIComputeShader>         TiledLightDebugShader;

    TSharedRef<CRHIComputePipelineState> ReduceDepthInitalPSO;
    TSharedRef<CRHIComputeShader>        ReduceDepthInitalShader;

    TSharedRef<CRHIComputePipelineState> ReduceDepthPSO;
    TSharedRef<CRHIComputeShader>        ReduceDepthShader;
};