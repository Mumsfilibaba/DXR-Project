#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Shaders/ShadowShaders.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

class FRenderGraphBuilder;

struct FDirectionalShadowSettingsHLSL
{
    // 0-16
    float  FilterSize;
    float  MaxFilterSize;
    uint32 ShadowMapSize;
    uint32 FrameIndex;

    // 16-32
    uint32 NumSamples;
    uint32 Padding0;
    uint32 Padding1;
    uint32 Padding2;
};

MARK_AS_REALLOCATABLE(FDirectionalShadowSettingsHLSL);

class FShadowMaskRenderPass : public FRenderPass
{
public:
    NODISCARD static FShadowMaskCS::FPermutation    CreateCurrentPermutation();
    NODISCARD static FDirectionalShadowSettingsHLSL CreateShadowSettings(const FFrameResources& Resources, uint32 FrameIndex);

public:
    FShadowMaskRenderPass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskRenderPass();

    bool Initialize(FFrameResources& FrameResources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);
    bool RetrievePipelineState(const FShadowMaskCS::FPermutation& Permutation, FComputePipelineStateInstance& OutPSO);

    NODISCARD FRHIBuffer* GetShadowSettingsBuffer() const
    {
        return ShadowSettingsBuffer.Get();
    }

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& Resources, bool bForceDebugMode);

    TMap<int32, FComputePipelineStateInstance> PipelineStates;
    FRHIBufferRef                              ShadowSettingsBuffer;
};
