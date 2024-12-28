#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"

#define NUM_FRUSTUM_PLANES (6)

enum class ECubeMapRenderPassType : int32
{
    Unknown = 0,
    MultiPass,
    SinglePass,
    GeometryShaderSinglePass,

    First = MultiPass,
    Last = GeometryShaderSinglePass,
};

enum class ECascadeRenderPassType : int32
{
    Unknown = 0,
    MultiPass,
    SinglePass,
    GeometryShaderSinglePass,
    ViewInstancingSinglePass,

    First = MultiPass,
    Last = ViewInstancingSinglePass,
};

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

struct FShadowPerObjectHLSL
{
    // 0-64
    FMatrix4 WorldMatrix;
};

MARK_AS_REALLOCATABLE(FShadowPerObjectHLSL);

struct FSinglePassPointLightBufferHLSL
{
    // 0-384
    FMatrix4 LightProjections[RHI_NUM_CUBE_FACES];

    // 384-400
    FVector3 LightPosition;
    float    LightFarPlane;
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

struct FPointLightShaderCombination
{
    FPointLightShaderCombination()
        : Hash(0)
    {
    }

    bool operator==(const FPointLightShaderCombination& Other) const
    {
        return Hash == Other.Hash;
    }

    bool operator!=(const FPointLightShaderCombination& Other) const
    {
        return Hash == Other.Hash;
    }

    friend uint64 GetHashForType(const FPointLightShaderCombination& Value)
    {
        return Value.Hash;
    }

    union
    {
        struct
        {
            // Type of RenderPass
            ECubeMapRenderPassType RenderPassType;
            
            // Material-flags
            int32 MaterialFlags;
        };

        uint64 Hash;
    };
};

class FPointLightRenderPass : public FRenderPass
{
public:
    FPointLightRenderPass(FSceneRenderer* InRenderer);
    virtual ~FPointLightRenderPass();

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) override final { }

    // This function creates or retrieves a pipeline state instance based on parameters
    FGraphicsPipelineStateInstance* CompilePipelineStateInstance(ECubeMapRenderPassType RenderPassType, FMaterial* Material, const FFrameResources& FrameResources);

    bool Initialize(FFrameResources& Resources);
    bool CreateResources(FFrameResources& Resources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

private:
    template<ECubeMapRenderPassType RenderPassType>
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    // Cache PipelineState types
    TMap<FPointLightShaderCombination, FGraphicsPipelineStateInstance> MaterialPSOs;

    // Buffers
    FRHIBufferRef PerShadowMapBuffer;
    FRHIBufferRef SinglePassShadowMapBuffer;
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

struct FCascadedShadowsShaderCombination
{
    FCascadedShadowsShaderCombination()
        : Hash(0)
    {
    }

    bool operator==(const FCascadedShadowsShaderCombination& Other) const
    {
        return Hash == Other.Hash;
    }

    bool operator!=(const FCascadedShadowsShaderCombination& Other) const
    {
        return Hash == Other.Hash;
    }

    friend uint64 GetHashForType(const FCascadedShadowsShaderCombination& Value)
    {
        return Value.Hash;
    }

    union
    {
        struct
        {
            // Type of RenderPass
            ECascadeRenderPassType RenderPassType;

            // Material-flags
            int32 MaterialFlags;
        };

        uint64 Hash;
    };
};

class FCascadedShadowsRenderPass : public FRenderPass
{
public:
    FCascadedShadowsRenderPass(FSceneRenderer* InRenderer);
    virtual ~FCascadedShadowsRenderPass();

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) override final { }

    // This function creates or retrieves a pipeline state instance based on parameters
    FGraphicsPipelineStateInstance* CompilePipelineStateInstance(ECascadeRenderPassType RenderPassType, FMaterial* Material, const FFrameResources& FrameResources);

    bool Initialize(FFrameResources& Resources);
    bool CreateResources(FFrameResources& Resources);
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

private:
    template<ECascadeRenderPassType RenderPassType>
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    TMap<FCascadedShadowsShaderCombination, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef PerCascadeBuffer;
};

struct FDirectionalShadowSettingsHLSL
{
    // 0-16
    float  FilterSize;
    float  MaxFilterSize;
    uint32 ShadowMapSize;
    uint32 FrameIndex;
};

MARK_AS_REALLOCATABLE(FDirectionalShadowSettingsHLSL);

enum ECSMFilterFunction
{
    CSMFilterFunction_GridPCF = 0,
    CSMFilterFunction_PoissonDiscPCF = 1,
};

struct FShadowMaskShaderCombination
{
    FShadowMaskShaderCombination()
        : Hash(0)
    {
    }

    bool operator==(const FShadowMaskShaderCombination& Other) const
    {
        return Hash == Other.Hash;
    }

    bool operator!=(const FShadowMaskShaderCombination& Other) const
    {
        return Hash == Other.Hash;
    }

    friend uint64 GetHashForType(const FShadowMaskShaderCombination& Value)
    {
        return Value.Hash;
    }

    union
    {
        struct
        {
            // Filter Function to use
            uint64 FilterFunction : 2;

            // Rotate the samples when using Poisson Disc
            uint64 bRotateSamples : 1;

            // DebugMode
            uint64 bDebugMode : 1;

            // Select cascade from projection instead of ViewZ
            uint64 bSelectCascadeFromProjection : 1;

            // BlendBetween cascades
            uint64 bBlendCascades : 1;

            // Number of Poisson samples
            uint64 NumPoissonSamples : 8;
        };

        uint64 Hash;
    };
};

class FShadowMaskRenderPass : public FRenderPass
{
public:
    FShadowMaskRenderPass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskRenderPass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources);
    bool RetrievePipelineState(const FShadowMaskShaderCombination& Combination, FComputePipelineStateInstance& OutPSO);
    void RetrieveCurrentCombinationBasedOnCVar(FShadowMaskShaderCombination& OutCombination);

private:
    TMap<FShadowMaskShaderCombination, FComputePipelineStateInstance> PipelineStates;
    FRHIBufferRef ShadowSettingsBuffer;
};
