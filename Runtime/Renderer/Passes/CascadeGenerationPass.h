#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"

#define NUM_FRUSTUM_PLANES (6)

struct FCascadeMatricesHLSL
{
    // 0-64
    FMatrix4 View;

    // 64-128
    FMatrix4 ViewProjection;

    // 128-196
    FMatrix4 InvView;

    // 196-256
    FMatrix4 InvViewProjection;
};

MARK_AS_REALLOCATABLE(FCascadeMatricesHLSL);

struct FCascadeSplitHLSL
{
    // 0-64
    FVector4 FrustumPlanes[NUM_FRUSTUM_PLANES];

    // 64-96
    FVector4 Offsets;
    FVector4 Scale;

    // 96-128
    FVector3 MinExtent;
    float    Split;
    FVector3 MaxExtent;
    float    NearPlane;

    // 128-144
    float   FarPlane;
    float   MinDepth;
    float   MaxDepth;
    float   PreviousSplit;

    // 144-160
    FVector3 CascadeCameraPosition;
    float    Padding0;
};

MARK_AS_REALLOCATABLE(FCascadeSplitHLSL);

class FRenderGraphBuilder;

class FCascadeGenerationPass : public FRenderPass
{
public:
    FCascadeGenerationPass(FSceneRenderer* InRenderer);
    virtual ~FCascadeGenerationPass();

    bool Initialize(FFrameResources& Resources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

private:
    void Record(FRHICommandList& CommandList, FFrameResources& FrameResources);

    FRHIComputePipelineStateRef CascadeGen;
    FRHIComputeShaderRef        CascadeGenShader;
};
