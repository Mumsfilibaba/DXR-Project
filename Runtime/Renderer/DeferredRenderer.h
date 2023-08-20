#pragma once
#include "FrameResources.h"
#include "LightSetup.h"
#include "Engine/Scene/Scene.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"

class RENDERER_API FDeferredRenderer
{
public:
    bool Init(FFrameResources& FrameResources);

    void Release();

    void RenderPrePass(FRHICommandList& CommandList, FFrameResources& FrameResources, const FScene& Scene);
    
    void RenderBasePass(FRHICommandList& CommandList, const FFrameResources& FrameResources);

    void RenderDeferredTiledLightPass(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup);

    bool ResizeResources(FFrameResources& FrameResources);

private:
    bool CreateGBuffer(FFrameResources& FrameResources);

    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          BaseVertexShader;
    FRHIPixelShaderRef           BasePixelShader;

    FRHIGraphicsPipelineStateRef PrePassPipelineState;
    FRHIVertexShaderRef          PrePassVertexShader;
    FRHIPixelShaderRef           PrePassPixelShader;

    FRHIComputePipelineStateRef  TiledLightPassPSO;
    FRHIComputeShaderRef         TiledLightShader;
    FRHIComputePipelineStateRef  TiledLightPassPSO_TileDebug;
    FRHIComputeShaderRef         TiledLightShader_TileDebug;
    FRHIComputePipelineStateRef  TiledLightPassPSO_CascadeDebug;
    FRHIComputeShaderRef         TiledLightShader_CascadeDebug;

    FRHIComputePipelineStateRef  ReduceDepthInitalPSO;
    FRHIComputeShaderRef         ReduceDepthInitalShader;

    FRHIComputePipelineStateRef  ReduceDepthPSO;
    FRHIComputeShaderRef         ReduceDepthShader;
};