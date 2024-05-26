#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "Core/Containers/Map.h"
#include "Engine/World/World.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"

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
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources);

private:
    FRHIComputePipelineStateRef TiledLightPassPSO;
    FRHIComputeShaderRef        TiledLightShader;
    FRHIComputePipelineStateRef TiledLightPassPSO_TileDebug;
    FRHIComputeShaderRef        TiledLightShader_TileDebug;
    FRHIComputePipelineStateRef TiledLightPassPSO_CascadeDebug;
    FRHIComputeShaderRef        TiledLightShader_CascadeDebug;
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

class FOcclusionPass : public FRenderPass
{
public:
    FOcclusionPass(FSceneRenderer* InRenderer);
    virtual ~FOcclusionPass();

    bool Initialize(FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene);

private:
    FRHIVertexShaderRef          VertexShader;
    FRHIGraphicsPipelineStateRef PipelineState;
};