#pragma once
#include "Core/Memory/MemoryStack.h"
#include "RendererCore/RenderGraph/RenderGraphPass.h"
#include "RendererCore/RenderGraph/RenderGraphResources.h"
#include "RendererCore/RenderGraph/RenderGraphViews.h"

struct FRenderGraphStatistics
{
    int32 NumPasses                  = 0;
    int32 NumCulledPasses            = 0;
    int32 NumDisabledPasses          = 0;
    int32 NumTransitionBarriers      = 0;
    int32 NumUnorderedAccessBarriers = 0;
    int32 NumSubresourceBarriers     = 0;
    int32 NumTexturesAllocated       = 0;
    int32 NumBuffersAllocated        = 0;
    int32 NumViewsCreated            = 0;
    int32 NumViewsCacheHits          = 0;
};

class RENDERERCORE_API FRenderGraphBuilder : FNonCopyable
{
public:
    explicit FRenderGraphBuilder(const CHAR* InName);
    ~FRenderGraphBuilder();

    NODISCARD FRenderGraphBuffer*              CreateBuffer(const FRenderGraphBufferDesc& Desc, const CHAR* Name);
    NODISCARD FRenderGraphTexture*             CreateTexture(const FRenderGraphTextureDesc& Desc, const CHAR* Name);
    NODISCARD FRenderGraphBuffer*              RegisterExternalBuffer(FRHIBuffer* Buffer, const CHAR* Name, ERHIResourceState InitialState, ERHIResourceState FinalState);
    NODISCARD FRenderGraphTexture*             RegisterExternalTexture(FRHITexture* Texture, const CHAR* Name, ERHIResourceState InitialState, ERHIResourceState FinalState);

    NODISCARD FRenderGraphShaderResourceView*  CreateSRV(FRenderGraphTexture* Texture, const FRHIShaderResourceViewDesc& Desc, const CHAR* Name);
    NODISCARD FRenderGraphUnorderedAccessView* CreateUAV(FRenderGraphTexture* Texture, const FRHIUnorderedAccessViewDesc& Desc, const CHAR* Name);
    NODISCARD FRenderGraphRenderTargetView*    CreateRTV(FRenderGraphTexture* Texture, const FRHIRenderTargetViewDesc& Desc, const CHAR* Name);
    NODISCARD FRenderGraphDepthStencilView*    CreateDSV(FRenderGraphTexture* Texture, const FRHIDepthStencilViewDesc& Desc, const CHAR* Name);

    NODISCARD FRenderGraphShaderResourceView*  CreateSRV(FRenderGraphBuffer* Buffer, const FRHIShaderResourceViewDesc& Desc, const CHAR* Name);
    NODISCARD FRenderGraphUnorderedAccessView* CreateUAV(FRenderGraphBuffer* Buffer, const FRHIUnorderedAccessViewDesc& Desc, const CHAR* Name);

    NODISCARD FRenderGraphShaderResourceView*  CreateSRV(FRenderGraphTexture* Texture, const CHAR* Name);
    NODISCARD FRenderGraphUnorderedAccessView* CreateUAV(FRenderGraphTexture* Texture, const CHAR* Name);
    NODISCARD FRenderGraphRenderTargetView*    CreateRTV(FRenderGraphTexture* Texture, const CHAR* Name);
    NODISCARD FRenderGraphDepthStencilView*    CreateDSV(FRenderGraphTexture* Texture, const CHAR* Name);

    NODISCARD FRenderGraphShaderResourceView*  GetOrCreateDefaultSRV(FRenderGraphTexture* Texture);
    NODISCARD FRenderGraphUnorderedAccessView* GetOrCreateDefaultUAV(FRenderGraphTexture* Texture);
    NODISCARD FRenderGraphRenderTargetView*    GetOrCreateDefaultRTV(FRenderGraphTexture* Texture);
    NODISCARD FRenderGraphDepthStencilView*    GetOrCreateDefaultDSV(FRenderGraphTexture* Texture, EDepthStencilViewFlags Flags = EDepthStencilViewFlags::None);
    NODISCARD FRenderGraphShaderResourceView*  GetOrCreateDefaultSRV(FRenderGraphBuffer* Buffer);
    NODISCARD FRenderGraphUnorderedAccessView* GetOrCreateDefaultUAV(FRenderGraphBuffer* Buffer);

    void Compile();
    void Execute(FRHICommandList& CommandList);

    template<typename SetupLambdaType, typename ExecuteLambdaType>
    void AddPass(const CHAR* Name, ERenderGraphPassFlags Flags, SetupLambdaType&& SetupLambda, ExecuteLambdaType&& ExecuteLambda)
    {
        AddPass(Name, Flags, true, Forward<SetupLambdaType>(SetupLambda), Forward<ExecuteLambdaType>(ExecuteLambda));
    }

    template<typename SetupLambdaType, typename ExecuteLambdaType>
    void AddPass(const CHAR* Name, ERenderGraphPassFlags Flags, bool bEnabled, SetupLambdaType&& SetupLambda, ExecuteLambdaType&& ExecuteLambda)
    {
        FRenderGraphPass* Pass = AllocatePass(Name, Flags, bEnabled);
        if (!Pass)
        {
            return;
        }

        FRenderGraphPassBuilder PassBuilder(*Pass, *this);
        SetupLambda(PassBuilder);

        typedef TRenderGraphPassExecutor<typename TRemoveReference<ExecuteLambdaType>::Type> ExecutorType;

        void* ExecutorMemory = Memory.Allocate(sizeof(ExecutorType), alignof(ExecutorType));
        Pass->SetExecutor(new(ExecutorMemory) ExecutorType(Forward<ExecuteLambdaType>(ExecuteLambda)));
    }

    NODISCARD const FRenderGraphStatistics& GetStatistics() const
    {
        return Statistics;
    }

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

    NODISCARD bool HasErrors() const
    {
        return bHasErrors;
    }

    void SetHasErrors(bool bInHasErrors = true)
    {
        bHasErrors = bInHasErrors;
    }

    NODISCARD const TArray<FRenderGraphPass*>& GetPasses() const
    {
        return Passes;
    }

    NODISCARD const TArray<FRenderGraphTexture*>& GetTextures() const
    {
        return Textures;
    }

    NODISCARD const TArray<FRenderGraphBuffer*>& GetBuffers() const
    {
        return Buffers;
    }

private:
    friend class FRenderGraphPassBuilder;

    NODISCARD static bool IsAccessPlannedThroughView(const FRenderGraphPass& Pass, FRenderGraphTexture* Texture, ERHIResourceState State, bool bIsWrite);

    NODISCARD static bool IsAccessPlannedThroughView(const FRenderGraphPass& Pass, FRenderGraphBuffer* Buffer, ERHIResourceState State, bool bIsWrite);

    NODISCARD FRenderGraphPass* AllocatePass(const CHAR* InName, ERenderGraphPassFlags InFlags, bool bEnabled = true);

    void CullPasses();
    void ResolveLifetimes();
    void AllocateResources();
    void ValidateGraph();
    void PlanBarriers();
    void EmitEpilogueBarriers(FRHICommandList& CommandList);
    void ReleasePooledResources();

    void PlanTextureViewBarrier(FRenderGraphPass* Pass, FRHITexture* RHITexture, FRenderGraphResourceState& State, const FRHITextureSubresourceRange& Range, ERHIResourceState AccessState, bool bIsWrite);

    void PlanBufferViewBarrier(FRenderGraphPass* Pass, FRHIBuffer* RHIBuffer, FRenderGraphResourceState& State, const FBufferRegion& Range, ERHIResourceState AccessState, bool bIsWrite);

    NODISCARD bool HasLiveOutput(const FRenderGraphPass& Pass) const;
    NODISCARD FRHIBeginRenderPassDesc BuildBeginRenderPassDesc(const FRenderGraphPass& Pass, FRenderGraphViewCache& ViewCache) const;

    template<typename ViewType, typename DescType>
    NODISCARD ViewType* AllocateTextureView(FRenderGraphTexture* Texture, const DescType& Desc, const CHAR* Name);

    template<typename ViewType, typename DescType>
    NODISCARD ViewType* AllocateShaderAccessTextureView(FRenderGraphTexture* Texture, const DescType& Desc, const CHAR* Name);

    template<typename ViewType, typename DescType>
    NODISCARD ViewType* AllocateBufferView(FRenderGraphBuffer* Buffer, const DescType& Desc, const CHAR* Name);

    const CHAR*                              Name;
    FMemoryStack                             Memory;
    TArray<FRenderGraphPass*>                Passes;
    TArray<FRenderGraphTexture*>             Textures;
    TArray<FRenderGraphBuffer*>              Buffers;
    TArray<FRenderGraphShaderResourceView*>  ShaderResourceViews;
    TArray<FRenderGraphUnorderedAccessView*> UnorderedAccessViews;
    TArray<FRenderGraphRenderTargetView*>    RenderTargetViews;
    TArray<FRenderGraphDepthStencilView*>    DepthStencilViews;
    TArray<FRenderGraphShaderResourceView*>  DefaultShaderResourceViews;
    TArray<FRenderGraphUnorderedAccessView*> DefaultUnorderedAccessViews;
    TArray<FRenderGraphRenderTargetView*>    DefaultRenderTargetViews;
    TArray<FRenderGraphDepthStencilView*>    DefaultDepthStencilViews;
    TArray<FRenderGraphShaderResourceView*>  DefaultBufferShaderResourceViews;
    TArray<FRenderGraphUnorderedAccessView*> DefaultBufferUnorderedAccessViews;
    FRenderGraphStatistics                   Statistics;
    bool                                     bIsCompiled;
    bool                                     bIsExecuted;
    bool                                     bHasErrors;
};
