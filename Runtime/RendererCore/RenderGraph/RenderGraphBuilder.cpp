#include "Core/Misc/OutputDeviceLogger.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/RenderGraph/RenderGraphResourcePool.h"

FRenderGraphBuilder::FRenderGraphBuilder(const CHAR* InName)
    : Name(InName ? InName : "RenderGraph")
    , Memory()
    , Passes()
    , Textures()
    , Buffers()
    , Statistics()
    , bIsCompiled(false)
    , bIsExecuted(false)
{
}

FRenderGraphBuilder::~FRenderGraphBuilder()
{
    for (FRenderGraphPass* Pass : Passes)
    {
        if (Pass->Executor)
        {
            Pass->Executor->~FRenderGraphPassExecutor();
        }

        Pass->~FRenderGraphPass();
    }

    if (bIsCompiled && !bIsExecuted)
    {
        ReleasePooledResources();
    }

    for (FRenderGraphTexture* Texture : Textures)
    {
        Texture->~FRenderGraphTexture();
    }

    for (FRenderGraphBuffer* Buffer : Buffers)
    {
        Buffer->~FRenderGraphBuffer();
    }

    Passes.Clear();
    Textures.Clear();
    Buffers.Clear();
    Memory.Reset();
}

FRenderGraphTexture* FRenderGraphBuilder::CreateTexture(const FRenderGraphTextureDesc& Desc, const CHAR* InName)
{
    void* TextureMemory = Memory.Allocate(sizeof(FRenderGraphTexture), alignof(FRenderGraphTexture));

    FRenderGraphTexture* Texture = new(TextureMemory) FRenderGraphTexture(Desc, InName, nullptr);
    Textures.Emplace(Texture);
    return Texture;
}

FRenderGraphBuffer* FRenderGraphBuilder::CreateBuffer(const FRenderGraphBufferDesc& Desc, const CHAR* InName)
{
    void* BufferMemory = Memory.Allocate(sizeof(FRenderGraphBuffer), alignof(FRenderGraphBuffer));

    FRenderGraphBuffer* Buffer = new(BufferMemory) FRenderGraphBuffer(Desc, InName, nullptr);
    Buffers.Emplace(Buffer);
    return Buffer;
}

FRenderGraphTexture* FRenderGraphBuilder::RegisterExternalTexture(FRHITexture* Texture, const CHAR* InName, ERHIResourceState InitialState, ERHIResourceState FinalState)
{
    if (!Texture)
    {
        LOG_ERROR("Graph '%s' cannot register a null external texture as '%s'", Name, InName ? InName : "Unnamed");
        return nullptr;
    }

    void* TextureMemory = Memory.Allocate(sizeof(FRenderGraphTexture), alignof(FRenderGraphTexture));

    FRenderGraphTexture* GraphTexture = new(TextureMemory) FRenderGraphTexture(FRenderGraphTextureDesc(Texture->GetDesc()), InName, Texture);
    GraphTexture->State.CurrentState  = InitialState;
    GraphTexture->State.FinalState    = FinalState;

    Textures.Emplace(GraphTexture);
    return GraphTexture;
}

FRenderGraphBuffer* FRenderGraphBuilder::RegisterExternalBuffer(FRHIBuffer* Buffer, const CHAR* InName, ERHIResourceState InitialState, ERHIResourceState FinalState)
{
    if (!Buffer)
    {
        LOG_ERROR("Graph '%s' cannot register a null external buffer as '%s'", Name, InName ? InName : "Unnamed");
        return nullptr;
    }

    void* BufferMemory = Memory.Allocate(sizeof(FRenderGraphBuffer), alignof(FRenderGraphBuffer));

    FRenderGraphBuffer* GraphBuffer = new(BufferMemory) FRenderGraphBuffer(FRenderGraphBufferDesc(Buffer->GetDesc()), InName, Buffer);
    GraphBuffer->State.CurrentState = InitialState;
    GraphBuffer->State.FinalState   = FinalState;

    Buffers.Emplace(GraphBuffer);
    return GraphBuffer;
}

FRenderGraphPass* FRenderGraphBuilder::AllocatePass(const CHAR* InName, ERenderGraphPassFlags InFlags)
{
    if (bIsCompiled)
    {
        LOG_ERROR("Graph '%s' cannot add pass '%s' after it has been compiled", Name, InName ? InName : "Unnamed");
        return nullptr;
    }

    void* PassMemory = Memory.Allocate(sizeof(FRenderGraphPass), alignof(FRenderGraphPass));

    FRenderGraphPass* Pass = new(PassMemory) FRenderGraphPass(InName, InFlags);
    Passes.Emplace(Pass);
    return Pass;
}

bool FRenderGraphBuilder::HasLiveOutput(const FRenderGraphPass& Pass) const
{
    for (const FRenderGraphTextureAccess& Access : Pass.TextureAccesses)
    {
        if (Access.bIsWrite && Access.Resource->State.NumReaders > 0)
        {
            return true;
        }
    }

    for (const FRenderGraphBufferAccess& Access : Pass.BufferAccesses)
    {
        if (Access.bIsWrite && Access.Resource->State.NumReaders > 0)
        {
            return true;
        }
    }

    return false;
}

void FRenderGraphBuilder::CullPasses()
{
    for (FRenderGraphTexture* Texture : Textures)
    {
        Texture->State.NumReaders = Texture->IsExternal() ? 1 : 0;
    }

    for (FRenderGraphBuffer* Buffer : Buffers)
    {
        Buffer->State.NumReaders = Buffer->IsExternal() ? 1 : 0;
    }

    for (FRenderGraphPass* Pass : Passes)
    {
        for (const FRenderGraphTextureAccess& Access : Pass->TextureAccesses)
        {
            if (!Access.bIsWrite)
            {
                ++Access.Resource->State.NumReaders;
            }
        }

        for (const FRenderGraphBufferAccess& Access : Pass->BufferAccesses)
        {
            if (!Access.bIsWrite)
            {
                ++Access.Resource->State.NumReaders;
            }
        }
    }

    bool bCulledAnyPass = true;
    while (bCulledAnyPass)
    {
        bCulledAnyPass = false;

        for (FRenderGraphPass* Pass : Passes)
        {
            if (Pass->bIsCulled || IsEnumFlagSet(Pass->Flags, ERenderGraphPassFlags::NeverCull))
            {
                continue;
            }

            if (HasLiveOutput(*Pass))
            {
                continue;
            }

            Pass->bIsCulled = true;
            bCulledAnyPass  = true;
            ++Statistics.NumCulledPasses;

            for (const FRenderGraphTextureAccess& Access : Pass->TextureAccesses)
            {
                if (!Access.bIsWrite)
                {
                    --Access.Resource->State.NumReaders;
                }
            }

            for (const FRenderGraphBufferAccess& Access : Pass->BufferAccesses)
            {
                if (!Access.bIsWrite)
                {
                    --Access.Resource->State.NumReaders;
                }
            }
        }
    }
}

void FRenderGraphBuilder::ResolveLifetimes()
{
    for (int32 PassIndex = 0; PassIndex < Passes.Size(); ++PassIndex)
    {
        const FRenderGraphPass* Pass = Passes[PassIndex];
        if (Pass->bIsCulled)
        {
            continue;
        }

        for (const FRenderGraphTextureAccess& Access : Pass->TextureAccesses)
        {
            FRenderGraphResourceState& State = Access.Resource->State;
            if (State.FirstPassIndex < 0)
            {
                State.FirstPassIndex = PassIndex;
            }

            State.LastPassIndex = PassIndex;
        }

        for (const FRenderGraphBufferAccess& Access : Pass->BufferAccesses)
        {
            FRenderGraphResourceState& State = Access.Resource->State;
            if (State.FirstPassIndex < 0)
            {
                State.FirstPassIndex = PassIndex;
            }

            State.LastPassIndex = PassIndex;
        }
    }
}

void FRenderGraphBuilder::AllocateResources()
{
    if (!FRenderGraphResourcePool::IsInitialized())
    {
        LOG_ERROR("Graph '%s' cannot allocate resources because the render-graph resource pool is not initialized", Name);
        return;
    }

    FRenderGraphResourcePool& Pool = FRenderGraphResourcePool::Get();

    for (FRenderGraphTexture* Texture : Textures)
    {
        // A resource no surviving pass touches never needs to exist
        if (Texture->IsExternal() || Texture->State.FirstPassIndex < 0)
        {
            continue;
        }

        Texture->Texture = Pool.AcquireTexture(Texture->Desc, Texture->Name, Texture->State.CurrentState);
        if (Texture->Texture)
        {
            ++Statistics.NumTexturesAllocated;
        }
    }

    for (FRenderGraphBuffer* Buffer : Buffers)
    {
        if (Buffer->IsExternal() || Buffer->State.FirstPassIndex < 0)
        {
            continue;
        }

        Buffer->Buffer = Pool.AcquireBuffer(Buffer->Desc, Buffer->Name, Buffer->State.CurrentState);
        if (Buffer->Buffer)
        {
            ++Statistics.NumBuffersAllocated;
        }
    }
}

void FRenderGraphBuilder::PlanBarriers()
{
    for (FRenderGraphPass* Pass : Passes)
    {
        if (Pass->bIsCulled)
        {
            continue;
        }

        for (const FRenderGraphTextureAccess& Access : Pass->TextureAccesses)
        {
            FRenderGraphTexture*       Texture = Access.Resource;
            FRenderGraphResourceState& State   = Texture->State;

            if (!Texture->GetRHITexture())
            {
                continue;
            }

            if (State.CurrentState == Access.State)
            {
                if (Access.State == ERHIResourceState::UnorderedAccess && State.bWrittenAsUnorderedAccess)
                {
                    Pass->UnorderedAccessBarriers.Emplace(FRHIUnorderedAccessBarrierDesc::CreateTexture(Texture->GetRHITexture()));
                    ++Statistics.NumUnorderedAccessBarriers;
                }
            }
            else
            {
                Pass->Transitions.Emplace(FRHITransitionBarrierDesc::CreateTexture(Texture->GetRHITexture(), State.CurrentState, Access.State));
                ++Statistics.NumTransitionBarriers;
                State.CurrentState = Access.State;
            }

            State.bWrittenAsUnorderedAccess = Access.bIsWrite && (Access.State == ERHIResourceState::UnorderedAccess);
        }

        for (const FRenderGraphBufferAccess& Access : Pass->BufferAccesses)
        {
            FRenderGraphBuffer*        Buffer = Access.Resource;
            FRenderGraphResourceState& State  = Buffer->State;

            if (!Buffer->GetRHIBuffer())
            {
                continue;
            }

            if (State.CurrentState == Access.State)
            {
                if (Access.State == ERHIResourceState::UnorderedAccess && State.bWrittenAsUnorderedAccess)
                {
                    Pass->UnorderedAccessBarriers.Emplace(FRHIUnorderedAccessBarrierDesc::CreateBuffer(Buffer->GetRHIBuffer()));
                    ++Statistics.NumUnorderedAccessBarriers;
                }
            }
            else
            {
                Pass->Transitions.Emplace(FRHITransitionBarrierDesc::CreateBuffer(Buffer->GetRHIBuffer(), State.CurrentState, Access.State));
                ++Statistics.NumTransitionBarriers;
                State.CurrentState = Access.State;
            }

            State.bWrittenAsUnorderedAccess = Access.bIsWrite && (Access.State == ERHIResourceState::UnorderedAccess);
        }
    }
}

void FRenderGraphBuilder::Compile()
{
    if (bIsCompiled)
    {
        return;
    }

    bIsCompiled = true;

    Statistics.NumPasses = Passes.Size();

    CullPasses();
    ResolveLifetimes();
    AllocateResources();
    PlanBarriers();
}

FRHIBeginRenderPassDesc FRenderGraphBuilder::BuildBeginRenderPassDesc(const FRenderGraphPass& Pass) const
{
    FRHIBeginRenderPassDesc::FRenderTargetAttachments RenderTargets;

    for (uint32 Index = 0; Index < Pass.NumRenderTargets; ++Index)
    {
        const FRenderGraphAttachment& Attachment = Pass.RenderTargets[Index];
        if (!Attachment.Texture || !Attachment.Texture->GetRHITexture())
        {
            continue;
        }

        RenderTargets[Index] = FRHIRenderPassAttachment(Attachment.Texture->GetRHITexture()->GetRenderTargetView(),
            Attachment.LoadAction, Attachment.StoreAction, Attachment.ClearValue);
    }

    FRHIDepthStencilAttachment DepthStencilAttachment;
    if (Pass.DepthStencil.Texture && Pass.DepthStencil.Texture->GetRHITexture())
    {
        DepthStencilAttachment = FRHIDepthStencilAttachment(Pass.DepthStencil.Texture->GetRHITexture()->GetDepthStencilView(),
            Pass.DepthStencil.LoadAction, Pass.DepthStencil.StoreAction, Pass.DepthStencil.DepthStencilClearValue);
    }

    return FRHIBeginRenderPassDesc(RenderTargets, Pass.NumRenderTargets, DepthStencilAttachment);
}

void FRenderGraphBuilder::EmitEpilogueBarriers(FRHICommandList& CommandList)
{
    TArray<FRHITransitionBarrierDesc> Transitions;

    for (FRenderGraphTexture* Texture : Textures)
    {
        if (!Texture->IsExternal() || Texture->State.CurrentState == Texture->State.FinalState)
        {
            continue;
        }

        Transitions.Emplace(FRHITransitionBarrierDesc::CreateTexture(Texture->GetRHITexture(),
            Texture->State.CurrentState, Texture->State.FinalState));

        Texture->State.CurrentState = Texture->State.FinalState;
    }

    for (FRenderGraphBuffer* Buffer : Buffers)
    {
        if (!Buffer->IsExternal() || Buffer->State.CurrentState == Buffer->State.FinalState)
        {
            continue;
        }

        Transitions.Emplace(FRHITransitionBarrierDesc::CreateBuffer(Buffer->GetRHIBuffer(),
            Buffer->State.CurrentState, Buffer->State.FinalState));

        Buffer->State.CurrentState = Buffer->State.FinalState;
    }

    if (!Transitions.IsEmpty())
    {
        CommandList.TransitionBarrier(MakeArrayView(Transitions));
        Statistics.NumTransitionBarriers += Transitions.Size();
    }
}

void FRenderGraphBuilder::ReleasePooledResources()
{
    if (!FRenderGraphResourcePool::IsInitialized())
    {
        return;
    }

    FRenderGraphResourcePool& Pool = FRenderGraphResourcePool::Get();
    for (FRenderGraphTexture* Texture : Textures)
    {
        if (!Texture->IsExternal())
        {
            Pool.ReleaseTexture(Texture->Texture, Texture->State.CurrentState);
        }
    }

    for (FRenderGraphBuffer* Buffer : Buffers)
    {
        if (!Buffer->IsExternal())
        {
            Pool.ReleaseBuffer(Buffer->Buffer, Buffer->State.CurrentState);
        }
    }
}

void FRenderGraphBuilder::Execute(FRHICommandList& CommandList)
{
    if (!bIsCompiled)
    {
        Compile();
    }

    if (bIsExecuted)
    {
        LOG_ERROR("Graph '%s' has already been executed. Build a new graph rather than replaying this one", Name);
        return;
    }

    bIsExecuted = true;

    RHI_EVENT_SCOPE(CommandList, Name);

    for (FRenderGraphPass* Pass : Passes)
    {
        if (Pass->bIsCulled)
        {
            continue;
        }

        RHI_EVENT_SCOPE(CommandList, Pass->Name);

        if (!Pass->Transitions.IsEmpty())
        {
            CommandList.TransitionBarrier(MakeArrayView(Pass->Transitions));
        }

        if (!Pass->UnorderedAccessBarriers.IsEmpty())
        {
            CommandList.UnorderedAccessBarrier(MakeArrayView(Pass->UnorderedAccessBarriers));
        }

        const bool bIsRaster = Pass->IsRaster();
        if (bIsRaster)
        {
            CommandList.BeginRenderPass(BuildBeginRenderPassDesc(*Pass));
        }

        if (Pass->Executor)
        {
            FRenderGraphPassResources Resources(*Pass);
            Pass->Executor->Execute(CommandList, Resources);
        }

        if (bIsRaster)
        {
            CommandList.EndRenderPass();
        }
    }

    EmitEpilogueBarriers(CommandList);
    ReleasePooledResources();

    if (FRenderGraphResourcePool::IsInitialized())
    {
        FRenderGraphResourcePool::Get().Tick();
    }
}
