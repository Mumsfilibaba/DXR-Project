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
    Matrix4 View;

    // 64-128
    Matrix4 ViewProjection;

    // 128-196
    Matrix4 InvView;

    // 196-256
    Matrix4 InvViewProjection;
};

MARK_AS_REALLOCATABLE(FCascadeMatricesHLSL);

struct FCascadeSplitHLSL
{
    // 0-64
    Vector4 FrustumPlanes[NUM_FRUSTUM_PLANES];

    // 64-96
    Vector4 Offsets;
    Vector4 Scale;

    // 96-128
    Vector3 MinExtent;
    float   Split;
    Vector3 MaxExtent;
    float   NearPlane;

    // 128-144
    float   FarPlane;
    float   MinDepth;
    float   MaxDepth;
    float   PreviousSplit;

    // 144-160
    Vector3 CascadeCameraPosition;
    float   Padding0;
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
