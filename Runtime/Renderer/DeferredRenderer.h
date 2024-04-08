#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "LightSetup.h"
#include "Core/Containers/Map.h"
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

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) override final;

    bool Initialize(FFrameResources& FrameResources);
    void Release();

    void RenderPrePass(FRHICommandList& CommandList, FFrameResources& FrameResources, FRendererScene* Scene);
    void RenderBasePass(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRendererScene* Scene);

    void RenderDeferredTiledLightPass(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup);

    bool ResizeResources(FRHICommandList& CommandList, FFrameResources& FrameResources, uint32 Width, uint32 Height);

private:
    bool CreateGBuffer(FFrameResources& FrameResources, uint32 Width, uint32 Height);

    // PrePass
    TMap<int32, FPipelineStateInstance> PrePassPSOs;

    // BasePass
    TMap<int32, FPipelineStateInstance> BasePassPSOs;

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