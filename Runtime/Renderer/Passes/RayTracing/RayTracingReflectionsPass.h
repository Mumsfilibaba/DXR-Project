#pragma once
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
    NODISCARD bool         IsTraceEnabled() const;
    NODISCARD FRHITexture* GetReflectionNoiseMask() const;

private:
    NODISCARD FRayTracingVariant         CreateVariant(const FRayTracingPermutation& Permutation);
    NODISCARD EReflectionPath            SelectPath() const;
    NODISCARD const FRayTracingVariant&  GetVariant(EReflectionPath Path) const;
    NODISCARD FRHIShaderBindingTableRef& GetShaderBindingTable(FFrameResources& Resources, EReflectionPath Path);
    NODISCARD uint32&                    GetHitGroupCapacity(EReflectionPath Path);

    void LoadReflectionNoiseMask();

    FRayTracingVariant          LocalVariant;
    FRayTracingVariant          BindlessVariant;
    FRayTracingVariant          SERVariant;
    FRHIComputeShaderRef        InlineReflectionsShader;
    FRHIComputePipelineStateRef InlineReflectionsPipeline;
    uint32                      CurrentSERHitGroupCapacity;
    uint32                      CurrentHitGroupCapacity;
    uint32                      CurrentBindlessHitGroupCapacity;
    FTextureRef                 ReflectionNoiseTexture;
    uint32                      ReflectionNoiseSize;
};
