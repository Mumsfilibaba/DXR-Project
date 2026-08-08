#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/ShadowShaders.h"
#include "Renderer/FrameResources.h"
#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneStaticMesh.h"

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

struct FPerCascadeHLSL
{
    // 0-16
    int32 CascadeIndex;
    int32 Padding0;
    int32 Padding1;
    int32 Padding2;
};

MARK_AS_REALLOCATABLE(FPerCascadeHLSL);

class FPointLightRenderPass : public FRenderPass
{
public:
    FPointLightRenderPass(FSceneRenderer* InRenderer);
    virtual ~FPointLightRenderPass();

    virtual void PreparePipelineState(FMaterial*, const FVertexDeclaration&, const FFrameResources&) override final { }

    FGraphicsPipelineStateInstance* CompilePipelineStateInstance(ECubeMapRenderPassType RenderPassType, const FMeshBatch& Batch, const FFrameResources& FrameResources);

    bool Initialize(FFrameResources& Resources);
    bool CreateResources(FFrameResources& Resources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

private:
    template<ECubeMapRenderPassType RenderPassType>
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef                                              PerShadowMapBuffer;
    FRHIBufferRef                                              SinglePassShadowMapBuffer;
};

class FCascadeGenerationPass : public FRenderPass
{
public:
    FCascadeGenerationPass(FSceneRenderer* InRenderer);
    virtual ~FCascadeGenerationPass();

    bool Initialize(FFrameResources& Resources);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources);

private:
    FRHIComputePipelineStateRef CascadeGen;
    FRHIComputeShaderRef        CascadeGenShader;
};

class FCascadedShadowsRenderPass : public FRenderPass
{
public:
    FCascadedShadowsRenderPass(FSceneRenderer* InRenderer);
    virtual ~FCascadedShadowsRenderPass();

    virtual void PreparePipelineState(FMaterial*, const FVertexDeclaration&, const FFrameResources&) override final { }

    FGraphicsPipelineStateInstance* CompilePipelineStateInstance(ECascadeRenderPassType RenderPassType, const FMeshBatch& Batch, const FFrameResources& FrameResources);

    bool Initialize(FFrameResources& Resources);
    bool CreateResources(FFrameResources& Resources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

private:
    template<ECascadeRenderPassType RenderPassType>
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    TMap<FGraphicsPipelineKey, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef                                              PerCascadeBuffer;
};

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
    NODISCARD static FShadowMaskCS::FPermutation CreateCurrentPermutation();

public:
    FShadowMaskRenderPass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskRenderPass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, bool bForceDebugMode = false);
    bool RetrievePipelineState(const FShadowMaskCS::FPermutation& Permutation, FComputePipelineStateInstance& OutPSO);

private:
    TMap<int32, FComputePipelineStateInstance> PipelineStates;
    FRHIBufferRef                              ShadowSettingsBuffer;
};
