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

struct FPerShadowMapHLSL
{
    // 0-64
    Matrix4 Matrix;

    // 64-80
    Vector3 Position;
    float   FarPlane;
};

MARK_AS_REALLOCATABLE(FPerShadowMapHLSL);

struct FSinglePassPointLightBufferHLSL
{
    // 0-384
    Matrix4 LightProjections[RHI_NUM_CUBE_FACES];

    // 384-400
    Vector3 LightPosition;
    float   LightFarPlane;
};

MARK_AS_REALLOCATABLE(FSinglePassPointLightBufferHLSL);

class FRenderGraphBuilder;

class FPointLightRenderPass : public FRenderPass
{
public:
    FPointLightRenderPass(FSceneRenderer* InRenderer);
    virtual ~FPointLightRenderPass();

    virtual void PreparePipelineState(FMaterial*, const FVertexDeclaration&, const FFrameResources&) override final { }

    FGraphicsPipelineStateInstance* CompilePipelineStateInstance(ECubeMapRenderPassType RenderPassType, const FMeshBatch& Batch, const FFrameResources& FrameResources);

    bool Initialize(FFrameResources& Resources);
    bool CreateResources(FFrameResources& Resources);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context);

    NODISCARD FRHIBuffer* GetPerShadowMapBuffer() const
    {
        return PerShadowMapBuffer.Get();
    }

    NODISCARD FRHIBuffer* GetSinglePassShadowMapBuffer() const
    {
        return SinglePassShadowMapBuffer.Get();
    }

private:
    void Record(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    template<ECubeMapRenderPassType RenderPassType>
    void RecordInternal(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef                                              PerShadowMapBuffer;
    FRHIBufferRef                                              SinglePassShadowMapBuffer;
};
