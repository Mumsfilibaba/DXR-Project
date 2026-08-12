#pragma once
#include "Core/Memory/MemoryStack.h"
#include "RendererCore/RenderGraph/RenderGraphPass.h"
#include "RendererCore/RenderGraph/RenderGraphResources.h"

struct FRenderGraphStatistics
{
    int32 NumPasses                  = 0;
    int32 NumCulledPasses            = 0;
    int32 NumTransitionBarriers      = 0;
    int32 NumUnorderedAccessBarriers = 0;
    int32 NumTexturesAllocated       = 0;
    int32 NumBuffersAllocated        = 0;
};

class RENDERERCORE_API FRenderGraphBuilder : FNonCopyable
{
public:
    explicit FRenderGraphBuilder(const CHAR* InName);
    ~FRenderGraphBuilder();

    NODISCARD FRenderGraphBuffer*  CreateBuffer(const FRenderGraphBufferDesc& Desc, const CHAR* Name);
    NODISCARD FRenderGraphTexture* CreateTexture(const FRenderGraphTextureDesc& Desc, const CHAR* Name);

    NODISCARD FRenderGraphBuffer*  RegisterExternalBuffer(FRHIBuffer* Buffer, const CHAR* Name, ERHIResourceState InitialState, ERHIResourceState FinalState);
    NODISCARD FRenderGraphTexture* RegisterExternalTexture(FRHITexture* Texture, const CHAR* Name, ERHIResourceState InitialState, ERHIResourceState FinalState);

    void Compile();
    void Execute(FRHICommandList& CommandList);

    template<typename SetupLambdaType, typename ExecuteLambdaType>
    void AddPass(const CHAR* Name, ERenderGraphPassFlags Flags, SetupLambdaType&& SetupLambda, ExecuteLambdaType&& ExecuteLambda)
    {
        FRenderGraphPass* Pass = AllocatePass(Name, Flags);
        if (!Pass)
        {
            return;
        }

        FRenderGraphPassBuilder PassBuilder(*Pass);
        SetupLambda(PassBuilder);

        typedef TRenderGraphPassExecutor<typename TRemoveReference<ExecuteLambdaType>::Type> ExecutorType;

        void* ExecutorMemory = Memory.Allocate(sizeof(ExecutorType), alignof(ExecutorType));
        Pass->Executor       = new(ExecutorMemory) ExecutorType(Forward<ExecuteLambdaType>(ExecuteLambda));
    }

    NODISCARD const FRenderGraphStatistics& GetStatistics() const
    {
        return Statistics;
    }

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

private:
    FRenderGraphPass* AllocatePass(const CHAR* InName, ERenderGraphPassFlags InFlags);

    void CullPasses();
    void ResolveLifetimes();
    void AllocateResources();
    void PlanBarriers();

    NODISCARD bool HasLiveOutput(const FRenderGraphPass& Pass) const;
    NODISCARD FRHIBeginRenderPassDesc BuildBeginRenderPassDesc(const FRenderGraphPass& Pass) const;

    void EmitEpilogueBarriers(FRHICommandList& CommandList);
    void ReleasePooledResources();

    const CHAR*                  Name;
    FMemoryStack                 Memory;
    TArray<FRenderGraphPass*>    Passes;
    TArray<FRenderGraphTexture*> Textures;
    TArray<FRenderGraphBuffer*>  Buffers;
    FRenderGraphStatistics       Statistics;
    bool                         bIsCompiled;
    bool                         bIsExecuted;
};
