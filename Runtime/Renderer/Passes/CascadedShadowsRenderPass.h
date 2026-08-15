#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Shaders/ShadowShaders.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

struct FPerCascadeHLSL
{
    // 0-16
    int32 CascadeIndex;
    int32 Padding0;
    int32 Padding1;
    int32 Padding2;
};

MARK_AS_REALLOCATABLE(FPerCascadeHLSL);

class FRenderGraphBuilder;

class FCascadedShadowsRenderPass : public FRenderPass
{
public:
    FCascadedShadowsRenderPass(FSceneRenderer* InRenderer);
    virtual ~FCascadedShadowsRenderPass();

    virtual void PreparePipelineState(FMaterial*, const FVertexDeclaration&, const FFrameResources&) override final { }

    FGraphicsPipelineStateInstance* CompilePipelineStateInstance(ECascadeRenderPassType RenderPassType, const FMeshBatch& Batch, const FFrameResources& FrameResources);
    
    bool Initialize(FFrameResources& Resources);
    bool CreateResources(FFrameResources& Resources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

    NODISCARD FRHIBuffer* GetPerCascadeBuffer() const
    {
        return PerCascadeBuffer.Get();
    }

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    template<ECascadeRenderPassType RenderPassType>
    void RecordInternal(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef                                              PerCascadeBuffer;
};
