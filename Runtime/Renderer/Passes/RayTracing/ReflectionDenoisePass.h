#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"

class FRenderGraphBuilder;
struct FSceneRenderGraphContext;

class FReflectionDenoisePass : public FRenderPass
{
public:
    // Matches the clamp applied to Renderer.RayTracing.Reflections.AtrousIterations
    static constexpr int32 MaxAtrousIterations = 8;

    FReflectionDenoisePass(FSceneRenderer* InRenderer);
    ~FReflectionDenoisePass();

    bool Initialize(FFrameResources& Resources);
    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    void Release();
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context, bool bTraceEnabled);

    NODISCARD bool NeedsReconfigure(const FFrameResources& Resources) const;
    NODISCARD bool IsDenoiseEnabled(const FFrameResources& Resources) const;

    void InvalidateHistory()
    {
        bHistoryValid = false;
    }

private:
    void RecordTemporal(FRHICommandList& CommandList, FFrameResources& Resources, uint32 ReadIndex, uint32 WriteIndex, FRHITexture* Target);
    void RecordAtrous(FRHICommandList& CommandList, FFrameResources& Resources, FRHITexture* Source, FRHITexture* Target, int32 Iteration);
    void RecordUpsample(FRHICommandList& CommandList, FFrameResources& Resources, FRHITexture* Source);

    FRHIComputeShaderRef        TemporalShader;
    FRHIComputePipelineStateRef TemporalPipeline;
    FRHIComputeShaderRef        AtrousShader;
    FRHIComputePipelineStateRef AtrousPipeline;
    FRHIComputeShaderRef        UpsampleShader;
    FRHIComputePipelineStateRef UpsamplePipeline;
    uint32                      HistoryIndex;
    bool                        bHistoryValid;
};
