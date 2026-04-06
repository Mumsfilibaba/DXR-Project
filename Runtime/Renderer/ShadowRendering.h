#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"
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

    First = MultiPass,
    Last = GeometryShaderSinglePass,
};

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
    float FarPlane;
    float MinDepth;
    float MaxDepth;
    float PreviousSplit;

    // 144-160
    FVector3 CascadeCameraPosition;
    // Reference world-space texel size for this cascade computed using the full camera clip range
    // (independent of tight-frustum depth min/max). Used for stable PCSS clamping in ShadowMaskGen.hlsl.
    float    RefWorldTexelSize;

    // 160-176
    float    PCFMarginTexels;
    float    PCSSMarginTexels;
    float    MaxPCSSRadiusTexels;
    float    MaxPCSSRadiusWorld;

    // 176-192
    float    MaxPCSSSearchWorld;
    float    TransitionWidthViewZ;
    float    TransitionMarginTexels;
    float    CascadeUpdatedThisFrame;

    // 192-208
    float    Padding1;
    float    Padding2;
    float    Padding3;
    float    Padding4;
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
    // 0-112
    FTransformBufferHLSL Transform;
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
            uint32 MaterialFlags;
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

            // True if depth-clipping should be enabled
            bool bEnableDepthClipping;

            // Enable shadow pancaking
            bool bShadowPancaking;

            // Material-flags
            uint32 MaterialFlags;
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
    void Execute(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);

private:
    template<ECascadeRenderPassType RenderPassType>
    void Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene);

    TMap<FCascadedShadowsShaderCombination, FGraphicsPipelineStateInstance> MaterialPSOs;
    FRHIBufferRef PerCascadeBuffer;

};

struct FDirectionalShadowSettingsHLSL
{
    // 0-16
    float  PCFFilterWorld;
    float  PCFMinFilterRadiusTexels;
    uint32 ShadowMapSize;
    uint32 FrameIndex;

    // 16-32 (PCSS tuning)
    float  PCSSRadiusScale;
    float  PCSSBlockerSearchScale;
    float  PCSSMinFilterRadiusTexels;
    float  PCSSPadding0;

    // 32-48
    float  PCSSBlockerSamplingClump;
    float  PCSSMaxPenumbraWorld;
    float  PCSSMaxSearchDistanceWorld;
    float  PCSSMinFilterMaxAngularDiameter;

    // 48-64
    float  PCSSBlockerSearchAngularDiameter;
    float  ShadowMaxDistance;
    float  ShadowMaxDistanceFade;
    float  Padding3;

    // 64-80
    uint32 ShadowDebugMode;
    uint32 ShadowDebugPadding0;
    uint32 ShadowDebugPadding1;
    uint32 ShadowDebugPadding2;
};

MARK_AS_REALLOCATABLE(FDirectionalShadowSettingsHLSL);

enum ECSMFilterMode : uint8
{
    PCF  = 0,
    PCSS = 1,
};

enum ECSMFilterFunction : uint8
{
    // NOTE: Value 0 used to be Grid; it is treated as Poisson for backwards compatibility.
    PoissonDisk              = 1,
    VogelDisk                = 2,
    InterleavedGradientNoise = 3,
};

enum class EShadowDebugMode : uint32
{
    None = 0,
    CascadeIndex,
    CascadeTransition,
    FilterMargin,
    PCSSRadiusClamp,
    CascadeUpdated,
    Containment,
    CascadeFallback,
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
        return Hash != Other.Hash;
    }

    friend uint64 GetHashForType(const FShadowMaskShaderCombination& Value)
    {
        return Value.Hash;
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

            // BlendBetween cascades
            bool bBlendCascades : 1;

            // Allow per-tap cascade fallback sampling
            bool bCascadeFallback : 1;

            // Resolve out-of-bounds taps through bounded global remap
            bool bPerTapGlobalRemap : 1;

            // Number of samples (Valid for poisson- and vogel-disk)
            uint8 NumSamples : 8;

            // Number of samples for PCSS blocker search
            uint8 NumBlockerSamples : 8;

            // Max number of coarser cascades to test when remapping out-of-bounds taps
            uint8 MaxCascadeRemapSteps : 4;
        };

        uint64 Hash;
    };
};

static_assert(sizeof(FShadowMaskShaderCombination) == sizeof(uint64), "FShadowMaskShaderCombination must have the same size as uint64");

class FShadowMaskRenderPass : public FRenderPass
{
public:
    FShadowMaskRenderPass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskRenderPass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, uint32 ShadowDebugMode, bool bUseHistory);
    bool RetrievePipelineState(const FShadowMaskShaderCombination& Combination, FComputePipelineStateInstance& OutPSO);
    void RetrieveCurrentCombinationBasedOnCVar(FShadowMaskShaderCombination& OutCombination);

private:
    TMap<FShadowMaskShaderCombination, FComputePipelineStateInstance> PipelineStates;
    FRHIBufferRef ShadowSettingsBuffer;
};

class FShadowMaskHistoryPass : public FRenderPass
{
public:
    FShadowMaskHistoryPass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskHistoryPass();

    bool Initialize(FFrameResources& FrameResources);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources);

private:
    FRHIComputePipelineStateRef HistoryPSO;
    FRHIComputeShaderRef        HistoryShader;
    FRHISamplerStateRef         LinearSampler;
    uint32                      CurrentHistoryIndex = 0;
};

class FShadowMaskDenoisePass : public FRenderPass
{
public:
    FShadowMaskDenoisePass(FSceneRenderer* InRenderer);
    virtual ~FShadowMaskDenoisePass();

    bool Initialize(FFrameResources& FrameResources);
    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    void Execute(FRHICommandList& CommandList, FFrameResources& FrameResources);

private:
    FRHIComputePipelineStateRef DenoisePSO;
    FRHIComputeShaderRef        DenoiseShader;
};
