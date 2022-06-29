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

    void RenderPrePass(FRHICommandList& CmdList, SFrameResources& FrameResources, const CScene& Scene);
    void RenderBasePass(FRHICommandList& CmdList, const SFrameResources& FrameResources);
    void RenderDeferredTiledLightPass(FRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup);

    bool ResizeResources(SFrameResources& FrameResources);

private:
    bool CreateGBuffer(SFrameResources& FrameResources);

    TSharedRef<FRHIGraphicsPipelineState> PipelineState;
    TSharedRef<FRHIVertexShader>          BaseVertexShader;
    TSharedRef<FRHIPixelShader>           BasePixelShader;

    TSharedRef<FRHIGraphicsPipelineState> PrePassPipelineState;
    TSharedRef<FRHIVertexShader>          PrePassVertexShader;

    TSharedRef<FRHIComputePipelineState>  TiledLightPassPSO;
    TSharedRef<FRHIComputeShader>         TiledLightShader;
    TSharedRef<FRHIComputePipelineState>  TiledLightPassPSODebug;
    TSharedRef<FRHIComputeShader>         TiledLightDebugShader;

    TSharedRef<FRHIComputePipelineState> ReduceDepthInitalPSO;
    TSharedRef<FRHIComputeShader>        ReduceDepthInitalShader;

    TSharedRef<FRHIComputePipelineState> ReduceDepthPSO;
    TSharedRef<FRHIComputeShader>        ReduceDepthShader;
};