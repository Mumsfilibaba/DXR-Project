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

    bool Init(FFrameResources& FrameResources);

    void Release();

    void RenderPrePass(FRHICommandList& CmdList, FFrameResources& FrameResources, const FScene& Scene);
    void RenderBasePass(FRHICommandList& CmdList, const FFrameResources& FrameResources);
    void RenderDeferredTiledLightPass(FRHICommandList& CmdList, const FFrameResources& FrameResources, const FLightSetup& LightSetup);

    bool ResizeResources(FFrameResources& FrameResources);

private:
    bool CreateGBuffer(FFrameResources& FrameResources);

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