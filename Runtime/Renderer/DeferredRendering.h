#pragma once
#include "Core/Containers/Map.h"
#include "Engine/World/World.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/FrameResources.h"
#include "Renderer/RenderPass.h"

class FDepthPrePass : public FRenderPass
{
public:
    FDepthPrePass(FSceneRenderer* InRenderer);
    virtual ~FDepthPrePass();

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) override final;
    
    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene);

private:
    void ExecuteInternal(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene, FRHITexture* DepthTarget, const char* PassName);

    TMap<int32, FGraphicsPipelineStateInstance> MaterialPSOs;
};

class FDeferredBasePass : public FRenderPass
{
public:
    FDeferredBasePass(FSceneRenderer* InRenderer);
    virtual ~FDeferredBasePass();

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) override final;

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene);

private:
    TMap<int32, FGraphicsPipelineStateInstance> MaterialPSOs;
};

class FTiledLightPass : public FRenderPass
{
public:
    FTiledLightPass(FSceneRenderer* InRenderer);
    virtual ~FTiledLightPass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

private:
    FRHIComputePipelineStateRef TiledLightPassPSO;
    FRHIComputeShaderRef        TiledLightShader;
    FRHIComputePipelineStateRef TiledLightPassPSO_TileDebug;
    FRHIComputeShaderRef        TiledLightShader_TileDebug;
};

class FDepthReducePass : public FRenderPass
{
public:
    FDepthReducePass(FSceneRenderer* InRenderer);
    virtual ~FDepthReducePass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene);

private:
    FRHIComputePipelineStateRef ReduceDepthInitalPSO;
    FRHIComputeShaderRef        ReduceDepthInitalShader;
    FRHIComputePipelineStateRef ReduceDepthPSO;
    FRHIComputeShaderRef        ReduceDepthShader;
};
