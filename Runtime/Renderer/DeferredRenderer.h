#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "Engine/Scene/Scene.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDeferredRenderer

class RENDERER_API FDeferredRenderer
{
public:
    FDeferredRenderer() = default;
    ~FDeferredRenderer() = default;

    bool Init(SFrameResources& FrameResources);

    void Release();

    void RenderPrePass(FRHICommandList& CmdList, SFrameResources& FrameResources, const FScene& Scene);
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

    FRHIComputePipelineStateRef  TiledLightPassPSO;
    FRHIComputeShaderRef         TiledLightShader;
    FRHIComputePipelineStateRef  TiledLightPassPSODebug;
    FRHIComputeShaderRef         TiledLightDebugShader;

    FRHIComputePipelineStateRef ReduceDepthInitalPSO;
    FRHIComputeShaderRef        ReduceDepthInitalShader;

    FRHIComputePipelineStateRef ReduceDepthPSO;
    FRHIComputeShaderRef        ReduceDepthShader;
};