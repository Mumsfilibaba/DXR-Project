#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "LightSetup.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"

#define NUM_FRUSTUM_PLANES (6)

struct FCascadeMatricesHLSL
{
    // 0-128
    FMatrix4 ViewProjection;
    FMatrix4 View;
};

MARK_AS_REALLOCATABLE(FCascadeMatricesHLSL);

struct FCascadeSplitHLSL
{
    // 0-96
    FVector4 FrustumPlanes[NUM_FRUSTUM_PLANES];
    // 96-112
    FVector4 Offsets;
    FVector4 Scale;
    // 112-128
    FVector3 MinExtent;
    float    Split;
    // 128-144
    FVector3 MaxExtent;
    float    NearPlane;
    // 144-160
    float FarPlane;
    float MinDepth;
    float MaxDepth;
    float PreviousSplit;
};

MARK_AS_REALLOCATABLE(FCascadeSplitHLSL);

struct FPerShadowMapHLSL
{
    // 0-64
    FMatrix4 Matrix;
    // 64-80
    FVector3 Position;
    float    FarPlane;
};

MARK_AS_REALLOCATABLE(FPerShadowMapHLSL);

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

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) override final;

    bool Initialize(FLightSetup& LightSetup);
    void Release();

    bool CreateResources(FLightSetup& LightSetup);

    void Execute(FRHICommandList& CommandList, const FLightSetup& LightSetup, FScene* Scene);

private:
    TMap<int32, FPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef PerShadowMapBuffer;
};

class FCascadeGenerationPass : public FRenderPass
{
public:
    FCascadeGenerationPass(FSceneRenderer* InRenderer);
    virtual ~FCascadeGenerationPass();

    bool Initialize(FLightSetup& LightSetup);
    void Release();

    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources, const FLightSetup& LightSetup);

private:
    FRHIComputePipelineStateRef CascadeGen;
    FRHIComputeShaderRef        CascadeGenShader;
};

class FCascadedShadowsRenderPass : public FRenderPass
{
public:
    FCascadedShadowsRenderPass(FSceneRenderer* InRenderer);
    virtual ~FCascadedShadowsRenderPass();

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) override final;

    bool Initialize(FLightSetup& LightSetup);
    void Release();

    bool CreateResources(FLightSetup& LightSetup);

    void Execute(FRHICommandList& CommandList, const FLightSetup& LightSetup, FScene* Scene);

private:
    TMap<int32, FPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef PerCascadeBuffer;
};

class FShadowMaskRenderPass : public FRenderPass
{
public:
    FShadowMaskRenderPass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskRenderPass();

    bool Initialize(const FFrameResources& FrameResources, FLightSetup& LightSetup);
    void Release();

    bool CreateResources(FLightSetup& LightSetup, uint32 Width, uint32 Height);
    bool ResizeResources(FRHICommandList& CommandList, FLightSetup& LightSetup, uint32 Width, uint32 Height);

    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup);

private:
    FRHIComputePipelineStateRef DirectionalShadowMaskPSO;
    FRHIComputeShaderRef        DirectionalShadowMaskShader;
    FRHIComputePipelineStateRef DirectionalShadowMaskPSO_Debug;
    FRHIComputeShaderRef        DirectionalShadowMaskShader_Debug;
};
