#pragma once
#include "FrameResources.h"
#include "LightSetup.h"
#include "Engine/Scene/Scene.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"

class RENDERER_API FDeferredRenderer
{
public:
    bool Initialize(FFrameResources& FrameResources);

    void Release();

    void RenderPrePass(FRHICommandList& CommandList, FFrameResources& FrameResources, const FScene& Scene);
    
    void RenderBasePass(FRHICommandList& CommandList, const FFrameResources& FrameResources);

    void RenderDeferredTiledLightPass(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup);

    bool ResizeResources(FFrameResources& FrameResources);

private:
    bool CreateGBuffer(FFrameResources& FrameResources);

    FRHIGraphicsPipelineStateRef BasePassPSO;
    FRHIGraphicsPipelineStateRef BasePassMaskedPSO;
    FRHIGraphicsPipelineStateRef BasePassDoubleSidedPSO;
    FRHIVertexShaderRef          BasePassVS;
    FRHIVertexShaderRef          BasePassMaskedVS;
    FRHIPixelShaderRef           BasePassPS;
    FRHIPixelShaderRef           BasePassMaskedPS;

    FRHIGraphicsPipelineStateRef PrePassPSO;
    FRHIGraphicsPipelineStateRef PrePassMaskedPSO;
    FRHIGraphicsPipelineStateRef PrePassDoubleSidedPSO;
    FRHIVertexShaderRef          PrePassVS;
    FRHIVertexShaderRef          PrePassMaskedVS;
    FRHIPixelShaderRef           PrePassMaskedPS;

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