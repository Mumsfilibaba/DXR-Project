#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "LightSetup.h"
#include "Engine/Scene/Scene.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"

class FDeferredRenderer : public FRenderPass
{
public:
    FDeferredRenderer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

    bool Initialize(FFrameResources& FrameResources);
    void Release();

    void RenderPrePass(FRHICommandList& CommandList, FFrameResources& FrameResources, FRendererScene* Scene);
    void RenderBasePass(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRendererScene* Scene);

    void RenderDeferredTiledLightPass(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup);

    bool ResizeResources(FRHICommandList& CommandList, FFrameResources& FrameResources, uint32 Width, uint32 Height);

private:
    bool CreateGBuffer(FFrameResources& FrameResources, uint32 Width, uint32 Height);

    // States for materials that use a single texture for each material-property
    FRHIGraphicsPipelineStateRef BasePassPSO;
    FRHIGraphicsPipelineStateRef BasePassMaskedPSO;
    FRHIGraphicsPipelineStateRef BasePassDoubleSidedPSO;
    FRHIGraphicsPipelineStateRef BasePassHeightPSO;
    FRHIVertexShaderRef          BasePassVS;
    FRHIVertexShaderRef          BasePassMaskedVS;
    FRHIVertexShaderRef          BasePassHeightVS;
    FRHIPixelShaderRef           BasePassPS;
    FRHIPixelShaderRef           BasePassMaskedPS;
    FRHIPixelShaderRef           BasePassHeightPS;

    // States for materials that use packed textures
    FRHIGraphicsPipelineStateRef BasePassPackedPSO;
    FRHIGraphicsPipelineStateRef BasePassPackedMaskedPSO;
    FRHIGraphicsPipelineStateRef BasePassPackedDoubleSidedPSO;
    FRHIGraphicsPipelineStateRef BasePassPackedHeightPSO;
    FRHIVertexShaderRef          BasePassPackedVS;
    FRHIVertexShaderRef          BasePassPackedMaskedVS;
    FRHIVertexShaderRef          BasePassPackedHeightVS;
    FRHIPixelShaderRef           BasePassPackedPS;
    FRHIPixelShaderRef           BasePassPackedMaskedPS;
    FRHIPixelShaderRef           BasePassPackedHeightPS;

    // States for materials that use a single texture for each material-property
    FRHIGraphicsPipelineStateRef PrePassPSO;
    FRHIGraphicsPipelineStateRef PrePassHeightPSO;
    FRHIGraphicsPipelineStateRef PrePassMaskedPSO;
    FRHIGraphicsPipelineStateRef PrePassDoubleSidedPSO;
    FRHIVertexShaderRef          PrePassVS;
    FRHIVertexShaderRef          PrePassHeightVS;
    FRHIPixelShaderRef           PrePassHeightPS;
    FRHIVertexShaderRef          PrePassMaskedVS;
    FRHIPixelShaderRef           PrePassMaskedPS;

    // States for materials that use packed textures
    FRHIGraphicsPipelineStateRef PrePassPackedMaskedPSO;
    FRHIGraphicsPipelineStateRef PrePassPackedDoubleSidedPSO;
    FRHIVertexShaderRef          PrePassPackedMaskedVS;
    FRHIPixelShaderRef           PrePassPackedMaskedPS;

    // Compute states for Deferred Light stage
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