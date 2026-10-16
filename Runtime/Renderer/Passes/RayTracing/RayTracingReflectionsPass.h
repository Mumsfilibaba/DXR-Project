#pragma once
#include "Core/Stats/Stats.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Resources/Texture.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"
#include "Renderer/Shaders/RayTracingShaders.h"

class FScene;
class FRenderGraphBuilder;
struct FSceneRenderGraphContext;

struct FRayTracingVariant
{
    NODISCARD explicit operator bool() const
    {
        return Pipeline != nullptr;
    }

    void Reset()
    {
        Pipeline.Reset();
        RayGenShader.Reset();
    }

    FRHIRayTracingPipelineStateRef Pipeline;
    FRHIRayGenShaderRef            RayGenShader;
};

enum class EReflectionPath : uint8
{
    Local,
    Bindless,
    ShaderExecutionReordering,
    Inline,
};

class FRayTracingReflectionsPass : public FRenderPass
{
public:
    FRayTracingReflectionsPass(FSceneRenderer* InRenderer);
    ~FRayTracingReflectionsPass();

    bool Initialize(FFrameResources& Resources);
    void Release();

    bool CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height);
    void AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context, bool bDenoise);
    void Record(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, bool bDenoise);

    NODISCARD bool         NeedsBindlessData() const;
    NODISCARD bool         UsesInlineRayTracing() const;
    NODISCARD bool         IsTraceEnabled() const;
    NODISCARD FRHITexture* GetReflectionNoiseMask() const;

private:
    NODISCARD FRayTracingVariant         CreateVariant(const FRayTracingPermutation& Permutation);
    NODISCARD EReflectionPath            SelectPath() const;
    NODISCARD const FRayTracingVariant&  GetVariant(EReflectionPath Path) const;
    NODISCARD FRHIShaderBindingTableRef& GetShaderBindingTable(FFrameResources& Resources, EReflectionPath Path);
    NODISCARD uint32&                    GetHitGroupCapacity(EReflectionPath Path);

    void LoadReflectionNoiseMask();

#if STATS_ENABLED
    void ClearAlphaTestCounter(FRHICommandList& CommandList);
    void ReadBackAlphaTestCounter(FRHICommandList& CommandList);

    static constexpr uint32 NumAlphaTestReadbacks = 3;
#else
    void ClearAlphaTestCounter(FRHICommandList&) { }
    void ReadBackAlphaTestCounter(FRHICommandList&) { }
#endif

    FRayTracingVariant          LocalVariant;
    FRayTracingVariant          BindlessVariant;
    FRayTracingVariant          SERVariant;
    FRHIComputeShaderRef        InlineReflectionsShader;
    FRHIComputePipelineStateRef InlineReflectionsPipeline;
    uint32                      CurrentSERHitGroupCapacity;
    uint32                      CurrentHitGroupCapacity;
    uint32                      CurrentBindlessHitGroupCapacity;
    TSharedPtr<FTexture>        ReflectionNoiseTexture;
    uint32                      ReflectionNoiseSize;

#if STATS_ENABLED
    FRHIBufferRef               AlphaTestCounter;
    FRHIUnorderedAccessViewRef  AlphaTestCounterUAV;
    FRHIBufferRef               AlphaTestReadbacks[NumAlphaTestReadbacks];
    uint32                      AlphaTestFrame;
#endif
};
