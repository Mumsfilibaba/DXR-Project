#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FRenderGraphBuilder;

extern bool GClearBeforeSkyboxEnabled;

class FSkyboxRenderPass : public FRenderPass
{
public:
    FSkyboxRenderPass(FSceneRenderer* InRenderer);
    virtual ~FSkyboxRenderPass();

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

    NODISCARD FRHIBuffer* GetVertexBuffer() const
    {
        return SkyboxVertexBuffer.Get();
    }

    NODISCARD FRHIBuffer* GetIndexBuffer() const
    {
        return SkyboxIndexBuffer.Get();
    }

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

    FRHIGraphicsPipelineStateRef PipelineState;
    FRHIVertexShaderRef          SkyboxVertexShader;
    FRHIPixelShaderRef           SkyboxPixelShader;
    FRHIBufferRef                SkyboxVertexBuffer;
    FRHIBufferRef                SkyboxIndexBuffer;
    uint32                       SkyboxIndexCount;
    EIndexFormat                 SkyboxIndexFormat;
    FRHISamplerStateRef          SkyboxSampler;
};
