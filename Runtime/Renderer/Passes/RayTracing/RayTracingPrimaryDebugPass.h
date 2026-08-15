#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"

class FScene;
class FRenderGraphBuilder;
struct FSceneRenderGraphContext;

class FRayTracingPrimaryDebugPass : public FRenderPass
{
public:
    FRayTracingPrimaryDebugPass(FSceneRenderer* InRenderer);
    ~FRayTracingPrimaryDebugPass();

    bool Initialize(FFrameResources& Resources);
    void Release();

    NODISCARD bool IsEnabled(const FSceneRenderView& SceneRenderView) const;
    
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);
    void Record(FRHICommandList& CommandList, FFrameResources& Resources);

private:
    FRHIComputeShaderRef        PrimaryRayDebugShader;
    FRHIComputePipelineStateRef PrimaryRayDebugPipeline;
};
