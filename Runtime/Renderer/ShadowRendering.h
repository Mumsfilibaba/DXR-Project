#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"
#include "Renderer/Scene/MeshBatch.h"
#include "Renderer/Scene/SceneStaticMesh.h"

#define NUM_FRUSTUM_PLANES (6)

enum class ECubeMapRenderPassType : uint8
{
    Unknown = 0,
    MultiPass,
    SinglePass,
    GeometryShaderSinglePass,

    First = MultiPass,
    Last = GeometryShaderSinglePass,
};

enum class ECascadeRenderPassType : uint8
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

    union
    {
        struct
        {
            // Type of RenderPass
            ECubeMapRenderPassType RenderPassType;

            // True when this PSO routes Albedo / Material sampler via SM 6.6 bindless heaps
            bool bBindless;

            // The vertex declaration whose input layout is baked into this PSO
            uint8 DeclarationID;

            // Explicit padding to keep the struct equal in size to Hash for stable hashing
            uint8 Padding0;

            // Material-flags, already narrowed to what the declaration can feed
            uint32 MaterialFlags;
        };

        uint64 Hash;
    };
};

static_assert(sizeof(FPointLightShaderCombination) == sizeof(uint64), "FPointLightShaderCombination must have the same size as uint64");

template<>
struct THash<FPointLightShaderCombination>
{
    static uint64 GetHash(const FPointLightShaderCombination& Value)
    {
        return Value.Hash;
    }
};

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

    TMap<FPointLightShaderCombination, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef                                                      PerShadowMapBuffer;
    FRHIBufferRef                                                      SinglePassShadowMapBuffer;
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

    union
    {
        struct
        {
            // Type of RenderPass
            ECascadeRenderPassType RenderPassType;

            // True if depth-clipping should be enabled
            bool bEnableDepthClipping;

            // True when this PSO routes Albedo / Material sampler via SM 6.6 bindless heaps
            bool bBindless;

            // The vertex declaration whose input layout is baked into this PSO
            uint8 DeclarationID;

            // Material-flags, already narrowed to what the declaration can feed
            uint32 MaterialFlags;
        };

        uint64 Hash;
    };
};

static_assert(sizeof(FCascadedShadowsShaderCombination) == sizeof(uint64), "FCascadedShadowsShaderCombination must have the same size as uint64");

template<>
struct THash<FCascadedShadowsShaderCombination>
{
    static uint64 GetHash(const FCascadedShadowsShaderCombination& Value)
    {
        return Value.Hash;
    }
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

    TMap<FCascadedShadowsShaderCombination, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef                                                           PerCascadeBuffer;
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

enum class ECSMFilterMode : uint8
{
    PCF  = 0,
    PCSS = 1,
};

enum class ECSMFilterFunction : uint8
{
    Grid        = 0,
    PoissonDisk = 1,
    VogelDisk   = 2,
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

    union
    {
        struct
        {
            // Filter Mode to use
            ECSMFilterMode FilterMode : 4;

            // Filter Function to use
            ECSMFilterFunction FilterFunction : 4;

            // Rotate the samples when using Poisson Disc
            bool bRotateSamples : 1;

            // DebugMode
            bool bDebugMode : 1;

            // Select cascade from projection instead of ViewZ
            bool bSelectCascadeFromProjection : 1;

            // BlendBetween cascades
            bool bBlendCascades : 1;

            // Number of samples (Valid for poisson- and vogel-disk)
            uint8 NumSamples : 8;
        };

        uint64 Hash;
    };
};

static_assert(sizeof(FShadowMaskShaderCombination) == sizeof(uint64), "FShadowMaskShaderCombination must have the same size as uint64");

template<>
struct THash<FShadowMaskShaderCombination>
{
    static uint64 GetHash(const FShadowMaskShaderCombination& Value)
    {
        return Value.Hash;
    }
};

class FShadowMaskRenderPass : public FRenderPass
{
public:
    FShadowMaskRenderPass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskRenderPass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, bool bForceDebugMode = false);
    bool RetrievePipelineState(const FShadowMaskShaderCombination& Combination, FComputePipelineStateInstance& OutPSO);
    void RetrieveCurrentCombinationBasedOnCVar(FShadowMaskShaderCombination& OutCombination);

private:
    TMap<FShadowMaskShaderCombination, FComputePipelineStateInstance> PipelineStates;
    FRHIBufferRef                                                     ShadowSettingsBuffer;
};
