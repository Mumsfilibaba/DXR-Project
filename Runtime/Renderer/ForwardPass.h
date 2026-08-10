#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"
#include "Renderer/Scene/MeshBatch.h"

class FForwardPass : public FRenderPass
{
public:
    FForwardPass(FSceneRenderer* InRenderer);
    virtual ~FForwardPass();

    bool Initialize(FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene);

private:
    FGraphicsPipelineStateInstance* CompilePipelineState(const FMaterialFeatures& Features, bool bBindless, const FVertexDeclaration& Declaration);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> PipelineStates;

    FRHIDepthStencilViewRef CachedReadOnlyDepthDSV;
    FRHITexture*            CachedReadOnlyDepthTarget;
};