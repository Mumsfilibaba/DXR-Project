#include "Core/Misc/OutputDeviceLogger.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/RenderGraph/RenderGraphResourcePool.h"
#include "RendererCore/RenderGraph/RenderGraphViewCache.h"
#include "RendererCore/RenderGraph/RenderGraphViewValidation.h"

struct FRenderGraphSubresourceSpan
{
    uint32 FirstMipLevel   = 0;
    uint32 NumMipLevels    = 1;
    uint32 FirstArraySlice = 0;
    uint32 NumArraySlices  = 1;
};

static bool IsAccessPlannedThroughViews(ERHIResourceState CoveredReadStates, bool bFoundMatchingWrite, ERHIResourceState State, bool bIsWrite)
{
    if (bIsWrite)
    {
        return bFoundMatchingWrite;
    }

    return (CoveredReadStates & State) == State;
}

static uint32 GetTrackedMipCount(const FRHITextureDesc& TextureDesc)
{
    return Math::Max<uint32>(TextureDesc.NumMipLevels, 1u);
}

static uint32 GetTrackedSliceCount(const FRHITextureDesc& TextureDesc)
{
    return Math::Max<uint32>(RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices), 1u);
}

static FRenderGraphSubresourceSpan ResolveSubresourceSpan(const FRHITextureSubresourceRange& Range, uint32 NumMipLevels, uint32 NumArraySlices)
{
    FRenderGraphSubresourceSpan Span;
    Span.FirstMipLevel   = Math::Min(Range.FirstMipLevel, NumMipLevels - 1);
    Span.FirstArraySlice = Math::Min(Range.FirstArraySlice, NumArraySlices - 1);

    Span.NumMipLevels = Range.NumMipLevels == RHI_ALL_MIP_LEVELS ? 
        NumMipLevels - Span.FirstMipLevel : 
        Math::Min(Range.NumMipLevels, NumMipLevels - Span.FirstMipLevel);

    Span.NumArraySlices = Range.NumArraySlices == RHI_ALL_ARRAY_SLICES ? 
        NumArraySlices - Span.FirstArraySlice : 
        Math::Min(Range.NumArraySlices, NumArraySlices - Span.FirstArraySlice);

    Span.NumMipLevels   = Math::Max<uint32>(Span.NumMipLevels, 1u);
    Span.NumArraySlices = Math::Max<uint32>(Span.NumArraySlices, 1u);
    return Span;
}

static FRHITextureSubresourceRange CreateSliceRunRange(uint32 MipLevel, uint32 FirstArraySlice, uint32 NumArraySlices)
{
    return FRHITextureSubresourceRange{ MipLevel, 1, FirstArraySlice, NumArraySlices, 0, RHI_ALL_PLANE_SLICES };
}

static FRenderGraphSubresourceState& GetSubresourceState(FRenderGraphResourceState& State, uint32 MipLevel, uint32 ArraySlice)
{
    return State.SubresourceStates[int32((MipLevel * State.NumTrackedArraySlices) + ArraySlice)];
}

static void BeginSubresourceTracking(FRenderGraphResourceState& State, uint32 NumMipLevels, uint32 NumArraySlices)
{
    if (State.bSubresourcesDiverged)
    {
        return;
    }

    FRenderGraphSubresourceState Initial;
    Initial.State                     = State.CurrentState;
    Initial.bWrittenAsUnorderedAccess = State.bWrittenAsUnorderedAccess;

    State.NumTrackedMipLevels   = NumMipLevels;
    State.NumTrackedArraySlices = NumArraySlices;

    State.SubresourceStates.Resize(int32(NumMipLevels * NumArraySlices));
    State.SubresourceStates.Fill(Initial);

    State.bSubresourcesDiverged = true;
}

static void SetUniformTextureState(FRenderGraphResourceState& State, ERHIResourceState NewState, bool bWrittenAsUnorderedAccess)
{
    State.CurrentState              = NewState;
    State.bWrittenAsUnorderedAccess = bWrittenAsUnorderedAccess;
    State.bSubresourcesDiverged     = false;
    State.NumTrackedMipLevels       = 0;
    State.NumTrackedArraySlices     = 0;

    State.SubresourceStates.Clear();
}

static bool TryGetUniformSubresourceState(const FRenderGraphResourceState& State, ERHIResourceState& OutState, bool& OutWrittenAsUnorderedAccess)
{
    if (State.SubresourceStates.IsEmpty())
    {
        return false;
    }

    const FRenderGraphSubresourceState& FirstSubresource = State.SubresourceStates.First();
    for (const FRenderGraphSubresourceState& Subresource : State.SubresourceStates)
    {
        if (Subresource.State != FirstSubresource.State || Subresource.bWrittenAsUnorderedAccess != FirstSubresource.bWrittenAsUnorderedAccess)
        {
            return false;
        }
    }

    OutState                    = FirstSubresource.State;
    OutWrittenAsUnorderedAccess = FirstSubresource.bWrittenAsUnorderedAccess;
    return true;
}

static FRHITransitionBarrierDesc CreateTextureTransition(FRHITexture* RHITexture, ERHIResourceState BeforeState, ERHIResourceState AccessState, const FRHITextureSubresourceRange& Range)
{
    const bool bIsTracked = RHITexture->GetDesc().TrackingMode == ERHIResourceStateTrackingMode::Tracked;
    return FRHITransitionBarrierDesc::CreateTextureSubresource(RHITexture, bIsTracked ? AccessState : BeforeState, AccessState, Range);
}

static void TransitionSubresourceSpan(FRHITexture* RHITexture, FRenderGraphResourceState& State, const FRenderGraphSubresourceSpan& Span, ERHIResourceState AccessState, 
    bool bIsWrite, bool bMustPlanBarrier, TArray<FRHITransitionBarrierDesc>& OutTransitions, TArray<FRHIUnorderedAccessBarrierDesc>& OutUnorderedAccessBarriers, FRenderGraphStatistics& Statistics)
{
    const uint32 LastArraySlice = Span.FirstArraySlice + Span.NumArraySlices;
    for (uint32 MipOffset = 0; MipOffset < Span.NumMipLevels; ++MipOffset)
    {
        const uint32 MipLevel = Span.FirstMipLevel + MipOffset;

        uint32 RunStart = Span.FirstArraySlice;
        while (RunStart < LastArraySlice)
        {
            const FRenderGraphSubresourceState& RunStartState            = GetSubresourceState(State, MipLevel, RunStart);
            const ERHIResourceState             BeforeState              = RunStartState.State;
            const bool                          bBeforeWasUnorderedWrite = RunStartState.bWrittenAsUnorderedAccess;

            uint32 RunEnd = RunStart + 1;
            while (RunEnd < LastArraySlice)
            {
                const FRenderGraphSubresourceState& Next = GetSubresourceState(State, MipLevel, RunEnd);
                if (Next.State != BeforeState || Next.bWrittenAsUnorderedAccess != bBeforeWasUnorderedWrite)
                {
                    break;
                }

                ++RunEnd;
            }

            const FRHITextureSubresourceRange RunRange = CreateSliceRunRange(MipLevel, RunStart, RunEnd - RunStart);
            if (BeforeState == AccessState && !bMustPlanBarrier)
            {
                if (AccessState == ERHIResourceState::UnorderedAccess && bBeforeWasUnorderedWrite)
                {
                    OutUnorderedAccessBarriers.Emplace(FRHIUnorderedAccessBarrierDesc::CreateTextureSubresource(RHITexture, RunRange));
                    ++Statistics.NumUnorderedAccessBarriers;
                    ++Statistics.NumSubresourceBarriers;
                }
            }
            else
            {
                OutTransitions.Emplace(CreateTextureTransition(RHITexture, BeforeState, AccessState, RunRange));
                ++Statistics.NumTransitionBarriers;
                ++Statistics.NumSubresourceBarriers;
            }

            for (uint32 ArraySlice = RunStart; ArraySlice < RunEnd; ++ArraySlice)
            {
                FRenderGraphSubresourceState& Subresource = GetSubresourceState(State, MipLevel, ArraySlice);
                Subresource.State                         = AccessState;
                Subresource.bWrittenAsUnorderedAccess     = bIsWrite && (AccessState == ERHIResourceState::UnorderedAccess);
            }

            RunStart = RunEnd;
        }
    }
}

static void TransitionTexture(FRHITexture* RHITexture, FRenderGraphResourceState& State, const FRHITextureSubresourceRange& Range, ERHIResourceState AccessState,
    bool bIsWrite, TArray<FRHITransitionBarrierDesc>& OutTransitions, TArray<FRHIUnorderedAccessBarrierDesc>& OutUnorderedAccessBarriers, FRenderGraphStatistics& Statistics)
{
    if (!RHITexture)
    {
        return;
    }

    const FRHITextureDesc& TextureDesc = RHITexture->GetDesc();
    if (TextureDesc.TrackingMode == ERHIResourceStateTrackingMode::Static)
    {
        return;
    }

    const uint32 NumMipLevels   = GetTrackedMipCount(TextureDesc);
    const uint32 NumArraySlices = GetTrackedSliceCount(TextureDesc);

    const bool bWrittenAsUnorderedAccess = bIsWrite && (AccessState == ERHIResourceState::UnorderedAccess);
    if (Range.IsAllSubresources())
    {
        const bool bMustPlanBarrier     = State.bInitialStateIsUnverified;
        State.bInitialStateIsUnverified = false;

        ERHIResourceState BeforeState              = State.CurrentState;
        bool              bBeforeWasUnorderedWrite = State.bWrittenAsUnorderedAccess;

        if (State.bSubresourcesDiverged && !TryGetUniformSubresourceState(State, BeforeState, bBeforeWasUnorderedWrite))
        {
            const FRenderGraphSubresourceSpan WholeSpan{ 0, NumMipLevels, 0, NumArraySlices };

            TransitionSubresourceSpan(RHITexture, State, WholeSpan, AccessState, bIsWrite, bMustPlanBarrier, OutTransitions, OutUnorderedAccessBarriers, Statistics);
            SetUniformTextureState(State, AccessState, bWrittenAsUnorderedAccess);
            return;
        }

        if (BeforeState == AccessState && !bMustPlanBarrier)
        {
            if (AccessState == ERHIResourceState::UnorderedAccess && bBeforeWasUnorderedWrite)
            {
                OutUnorderedAccessBarriers.Emplace(FRHIUnorderedAccessBarrierDesc::CreateTexture(RHITexture));
                ++Statistics.NumUnorderedAccessBarriers;
            }
        }
        else
        {
            OutTransitions.Emplace(CreateTextureTransition(RHITexture, BeforeState, AccessState, FRHITextureSubresourceRange::All()));
            ++Statistics.NumTransitionBarriers;
        }

        SetUniformTextureState(State, AccessState, bWrittenAsUnorderedAccess);
        return;
    }

    BeginSubresourceTracking(State, NumMipLevels, NumArraySlices);

    const FRenderGraphSubresourceSpan Span = ResolveSubresourceSpan(Range, NumMipLevels, NumArraySlices);
    TransitionSubresourceSpan(RHITexture, State, Span, AccessState, bIsWrite, State.bInitialStateIsUnverified, OutTransitions, OutUnorderedAccessBarriers, Statistics);

    ERHIResourceState UniformState              = AccessState;
    bool              bUniformWasUnorderedWrite = bWrittenAsUnorderedAccess;

    if (TryGetUniformSubresourceState(State, UniformState, bUniformWasUnorderedWrite))
    {
        SetUniformTextureState(State, UniformState, bUniformWasUnorderedWrite);
    }
}

static FRHIRenderTargetView* ResolveRenderTargetAttachmentView(const FRenderGraphAttachment& Attachment, FRenderGraphViewCache& ViewCache)
{
    if (Attachment.RenderTargetView)
    {
        if (FRHIRenderTargetView* RenderTargetView = ViewCache.GetOrCreate(Attachment.RenderTargetView))
        {
            return RenderTargetView;
        }
    }

    if (Attachment.Texture)
    {
        if (FRHITexture* Texture = Attachment.Texture->GetRHITexture())
        {
            return Texture->GetRenderTargetView();
        }
    }

    return nullptr;
}

static FRHIDepthStencilView* ResolveDepthStencilAttachmentView(const FRenderGraphDepthAttachment& DepthStencil, FRenderGraphViewCache& ViewCache)
{
    if (DepthStencil.DepthStencilView)
    {
        if (FRHIDepthStencilView* DepthStencilView = ViewCache.GetOrCreate(DepthStencil.DepthStencilView))
        {
            return DepthStencilView;
        }
    }

    if (DepthStencil.Texture)
    {
        if (FRHITexture* Texture = DepthStencil.Texture->GetRHITexture())
        {
            return Texture->GetDepthStencilView();
        }
    }

    return nullptr;
}

static bool HasValidRenderPassAttachments(const FRHIBeginRenderPassDesc& RenderPassDesc)
{
    for (uint32 Index = 0; Index < RenderPassDesc.NumRenderTargets; ++Index)
    {
        FRHIRenderTargetView* RenderTargetView = RenderPassDesc.RenderTargets[Index].View.Get();
        if (RenderTargetView && RenderTargetView->GetResource())
        {
            return true;
        }
    }

    FRHIDepthStencilView* DepthStencilView = RenderPassDesc.DepthStencilAttachment.View.Get();
    return DepthStencilView && DepthStencilView->GetResource();
}

FRenderGraphBuilder::FRenderGraphBuilder(const CHAR* InName)
    : Name(InName ? InName : "RenderGraph")
    , Memory()
    , Passes()
    , Textures()
    , Buffers()
    , ShaderResourceViews()
    , UnorderedAccessViews()
    , RenderTargetViews()
    , DepthStencilViews()
    , DefaultShaderResourceViews()
    , DefaultUnorderedAccessViews()
    , DefaultRenderTargetViews()
    , DefaultDepthStencilViews()
    , DefaultBufferShaderResourceViews()
    , DefaultBufferUnorderedAccessViews()
    , Statistics()
    , bIsCompiled(false)
    , bIsExecuted(false)
    , bHasErrors(false)
{
}

FRenderGraphBuilder::~FRenderGraphBuilder()
{
    for (FRenderGraphPass* Pass : Passes)
    {
        if (FRenderGraphPassExecutor* Executor = Pass->GetExecutor())
        {
            Executor->~FRenderGraphPassExecutor();
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

template<typename ViewType, typename DescType>
ViewType* FRenderGraphBuilder::AllocateTextureView(FRenderGraphTexture* Texture, const DescType& Desc, const CHAR* InName)
{
    void*     ViewMemory = Memory.Allocate(sizeof(ViewType), alignof(ViewType));
    ViewType* View       = new(ViewMemory) ViewType();

    View->Parent = Texture;
    View->Desc   = Desc;
    View->Name   = InName ? InName : "RenderGraphView";
    return View;
}

template<typename ViewType, typename DescType>
ViewType* FRenderGraphBuilder::AllocateShaderAccessTextureView(FRenderGraphTexture* Texture, const DescType& Desc, const CHAR* InName)
{
    void*     ViewMemory = Memory.Allocate(sizeof(ViewType), alignof(ViewType));
    ViewType* View       = new(ViewMemory) ViewType();

    View->Parent.Texture = Texture;
    View->ParentKind     = ERenderGraphParentKind::Texture;
    View->Desc           = Desc;
    View->Name           = InName ? InName : "RenderGraphView";
    return View;
}

template<typename ViewType, typename DescType>
ViewType* FRenderGraphBuilder::AllocateBufferView(FRenderGraphBuffer* Buffer, const DescType& Desc, const CHAR* InName)
{
    void*     ViewMemory = Memory.Allocate(sizeof(ViewType), alignof(ViewType));
    ViewType* View       = new(ViewMemory) ViewType();

    View->Parent.Buffer = Buffer;
    View->ParentKind    = ERenderGraphParentKind::Buffer;
    View->Desc          = Desc;
    View->Name          = InName ? InName : "RenderGraphView";
    return View;
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
    GraphTexture->State.CurrentState              = InitialState;
    GraphTexture->State.FinalState                = FinalState;
    GraphTexture->State.bInitialStateIsUnverified = Texture->GetDesc().TrackingMode == ERHIResourceStateTrackingMode::Tracked;

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

bool FRenderGraphBuilder::ValidateExternalViewRegistration(FRenderGraphTexture* Texture, const void* View, const CHAR* ViewKind, const CHAR* InName)
{
    if (!Texture)
    {
        LOG_ERROR("Graph '%s' cannot register the external %s '%s' without a texture to hang it on", Name, ViewKind, InName ? InName : "Unnamed");
        SetHasErrors(true);
        return false;
    }

    if (!View)
    {
        LOG_ERROR("Graph '%s' cannot register a null external %s as '%s'", Name, ViewKind, InName ? InName : "Unnamed");
        SetHasErrors(true);
        return false;
    }

    return true;
}

bool FRenderGraphBuilder::ValidateViewCreation(FRenderGraphTexture* Texture, const CHAR* ViewKind, const CHAR* InName)
{
    if (!Texture)
    {
        LOG_ERROR("Graph '%s' cannot create %s '%s' on a null texture", Name, ViewKind, InName ? InName : "Unnamed");
        SetHasErrors(true);
        return false;
    }

    if (Texture->GetDesc().TextureDesc.IsPresentable())
    {
        LOG_ERROR("Graph '%s' cannot create %s '%s' on back-buffer '%s'. Hand its own view to RegisterExternal%s instead",
            Name, ViewKind, InName ? InName : "Unnamed", Texture->GetName(), ViewKind);
        SetHasErrors(true);
        return false;
    }

    return true;
}

FRenderGraphShaderResourceView* FRenderGraphBuilder::RegisterExternalSRV(FRenderGraphTexture* Texture, FRHIShaderResourceView* ShaderResourceView, const CHAR* InName)
{
    if (!ValidateExternalViewRegistration(Texture, ShaderResourceView, "SRV", InName))
    {
        return nullptr;
    }

    FRenderGraphShaderResourceView* View = AllocateShaderAccessTextureView<FRenderGraphShaderResourceView>(Texture, ShaderResourceView->GetDesc(), InName);
    View->ExternalView = ShaderResourceView;
    ShaderResourceViews.Emplace(View);
    DefaultShaderResourceViews.Emplace(View);

    RenderGraphViewValidation::ValidateShaderResourceView(*this, View);
    return View;
}

FRenderGraphUnorderedAccessView* FRenderGraphBuilder::RegisterExternalUAV(FRenderGraphTexture* Texture, FRHIUnorderedAccessView* UnorderedAccessView, const CHAR* InName)
{
    if (!ValidateExternalViewRegistration(Texture, UnorderedAccessView, "UAV", InName))
    {
        return nullptr;
    }

    FRenderGraphUnorderedAccessView* View = AllocateShaderAccessTextureView<FRenderGraphUnorderedAccessView>(Texture, UnorderedAccessView->GetDesc(), InName);
    View->ExternalView = UnorderedAccessView;
    UnorderedAccessViews.Emplace(View);
    DefaultUnorderedAccessViews.Emplace(View);

    RenderGraphViewValidation::ValidateUnorderedAccessView(*this, View);
    return View;
}

FRenderGraphRenderTargetView* FRenderGraphBuilder::RegisterExternalRTV(FRenderGraphTexture* Texture, FRHIRenderTargetView* RenderTargetView, const CHAR* InName)
{
    if (!ValidateExternalViewRegistration(Texture, RenderTargetView, "RTV", InName))
    {
        return nullptr;
    }

    FRenderGraphRenderTargetView* View = AllocateTextureView<FRenderGraphRenderTargetView>(Texture, RenderTargetView->GetDesc(), InName);
    View->ExternalView = RenderTargetView;
    RenderTargetViews.Emplace(View);
    DefaultRenderTargetViews.Emplace(View);

    RenderGraphViewValidation::ValidateRenderTargetView(*this, View);
    return View;
}

FRenderGraphDepthStencilView* FRenderGraphBuilder::RegisterExternalDSV(FRenderGraphTexture* Texture, FRHIDepthStencilView* DepthStencilView, const CHAR* InName)
{
    if (!ValidateExternalViewRegistration(Texture, DepthStencilView, "DSV", InName))
    {
        return nullptr;
    }

    FRenderGraphDepthStencilView* View = AllocateTextureView<FRenderGraphDepthStencilView>(Texture, DepthStencilView->GetDesc(), InName);
    View->ExternalView = DepthStencilView;
    DepthStencilViews.Emplace(View);
    DefaultDepthStencilViews.Emplace(View);

    RenderGraphViewValidation::ValidateDepthStencilView(*this, View);
    return View;
}

FRenderGraphShaderResourceView* FRenderGraphBuilder::CreateSRV(FRenderGraphTexture* Texture, const FRHIShaderResourceViewDesc& Desc, const CHAR* InName)
{
    if (!ValidateViewCreation(Texture, "SRV", InName))
    {
        return nullptr;
    }

    FRenderGraphShaderResourceView* View = AllocateShaderAccessTextureView<FRenderGraphShaderResourceView>(Texture, Desc, InName);
    ShaderResourceViews.Emplace(View);

    RenderGraphViewValidation::ValidateShaderResourceView(*this, View);
    return View;
}

FRenderGraphUnorderedAccessView* FRenderGraphBuilder::CreateUAV(FRenderGraphTexture* Texture, const FRHIUnorderedAccessViewDesc& Desc, const CHAR* InName)
{
    if (!ValidateViewCreation(Texture, "UAV", InName))
    {
        return nullptr;
    }

    FRenderGraphUnorderedAccessView* View = AllocateShaderAccessTextureView<FRenderGraphUnorderedAccessView>(Texture, Desc, InName);
    UnorderedAccessViews.Emplace(View);

    RenderGraphViewValidation::ValidateUnorderedAccessView(*this, View);
    return View;
}

FRenderGraphRenderTargetView* FRenderGraphBuilder::CreateRTV(FRenderGraphTexture* Texture, const FRHIRenderTargetViewDesc& Desc, const CHAR* InName)
{
    if (!ValidateViewCreation(Texture, "RTV", InName))
    {
        return nullptr;
    }

    FRenderGraphRenderTargetView* View = AllocateTextureView<FRenderGraphRenderTargetView>(Texture, Desc, InName);
    RenderTargetViews.Emplace(View);

    RenderGraphViewValidation::ValidateRenderTargetView(*this, View);
    return View;
}

FRenderGraphDepthStencilView* FRenderGraphBuilder::CreateDSV(FRenderGraphTexture* Texture, const FRHIDepthStencilViewDesc& Desc, const CHAR* InName)
{
    if (!ValidateViewCreation(Texture, "DSV", InName))
    {
        return nullptr;
    }

    FRenderGraphDepthStencilView* View = AllocateTextureView<FRenderGraphDepthStencilView>(Texture, Desc, InName);
    DepthStencilViews.Emplace(View);

    RenderGraphViewValidation::ValidateDepthStencilView(*this, View);
    return View;
}

FRenderGraphShaderResourceView* FRenderGraphBuilder::CreateSRV(FRenderGraphBuffer* Buffer, const FRHIShaderResourceViewDesc& Desc, const CHAR* InName)
{
    if (!Buffer)
    {
        LOG_ERROR("Graph '%s' cannot create an SRV on a null buffer", Name);
        SetHasErrors(true);
        return nullptr;
    }

    FRenderGraphShaderResourceView* View = AllocateBufferView<FRenderGraphShaderResourceView>(Buffer, Desc, InName);
    ShaderResourceViews.Emplace(View);

    RenderGraphViewValidation::ValidateShaderResourceView(*this, View);
    return View;
}

FRenderGraphUnorderedAccessView* FRenderGraphBuilder::CreateUAV(FRenderGraphBuffer* Buffer, const FRHIUnorderedAccessViewDesc& Desc, const CHAR* InName)
{
    if (!Buffer)
    {
        LOG_ERROR("Graph '%s' cannot create a UAV on a null buffer", Name);
        SetHasErrors(true);
        return nullptr;
    }

    FRenderGraphUnorderedAccessView* View = AllocateBufferView<FRenderGraphUnorderedAccessView>(Buffer, Desc, InName);
    UnorderedAccessViews.Emplace(View);

    RenderGraphViewValidation::ValidateUnorderedAccessView(*this, View);
    return View;
}

FRenderGraphShaderResourceView* FRenderGraphBuilder::CreateSRV(FRenderGraphTexture* Texture, const CHAR* InName)
{
    return CreateSRV(Texture, RenderGraphDefaultViewDescs::ShaderResourceForTexture(Texture->GetDesc().TextureDesc), InName);
}

FRenderGraphUnorderedAccessView* FRenderGraphBuilder::CreateUAV(FRenderGraphTexture* Texture, const CHAR* InName)
{
    return CreateUAV(Texture, RenderGraphDefaultViewDescs::UnorderedAccessForTexture(Texture->GetDesc().TextureDesc), InName);
}

FRenderGraphRenderTargetView* FRenderGraphBuilder::CreateRTV(FRenderGraphTexture* Texture, const CHAR* InName)
{
    return CreateRTV(Texture, RenderGraphDefaultViewDescs::RenderTargetForTexture(Texture->GetDesc().TextureDesc), InName);
}

FRenderGraphDepthStencilView* FRenderGraphBuilder::CreateDSV(FRenderGraphTexture* Texture, const CHAR* InName)
{
    return CreateDSV(Texture, RenderGraphDefaultViewDescs::DepthStencilForTexture(Texture->GetDesc().TextureDesc), InName);
}

FRenderGraphShaderResourceView* FRenderGraphBuilder::GetOrCreateDefaultSRV(FRenderGraphTexture* Texture)
{
    for (FRenderGraphShaderResourceView* View : DefaultShaderResourceViews)
    {
        if (View->GetParentTexture() == Texture)
        {
            return View;
        }
    }

    FRenderGraphShaderResourceView* View = CreateSRV(Texture, "DefaultSRV");
    DefaultShaderResourceViews.Emplace(View);
    return View;
}

FRenderGraphUnorderedAccessView* FRenderGraphBuilder::GetOrCreateDefaultUAV(FRenderGraphTexture* Texture)
{
    for (FRenderGraphUnorderedAccessView* View : DefaultUnorderedAccessViews)
    {
        if (View->GetParentTexture() == Texture)
        {
            return View;
        }
    }

    FRenderGraphUnorderedAccessView* View = CreateUAV(Texture, "DefaultUAV");
    DefaultUnorderedAccessViews.Emplace(View);
    return View;
}

FRenderGraphRenderTargetView* FRenderGraphBuilder::GetOrCreateDefaultRTV(FRenderGraphTexture* Texture)
{
    for (FRenderGraphRenderTargetView* View : DefaultRenderTargetViews)
    {
        if (View->GetParent() == Texture)
        {
            return View;
        }
    }

    FRenderGraphRenderTargetView* View = CreateRTV(Texture, "DefaultRTV");
    DefaultRenderTargetViews.Emplace(View);
    return View;
}

FRenderGraphDepthStencilView* FRenderGraphBuilder::GetOrCreateDefaultDSV(FRenderGraphTexture* Texture, EDepthStencilViewFlags Flags)
{
    for (FRenderGraphDepthStencilView* View : DefaultDepthStencilViews)
    {
        if (View->GetParent() == Texture && View->GetDesc().Flags == Flags)
        {
            return View;
        }
    }

    FRHIDepthStencilViewDesc Desc = RenderGraphDefaultViewDescs::DepthStencilForTexture(Texture->GetDesc().TextureDesc);
    Desc.Flags = Flags;

    FRenderGraphDepthStencilView* View = CreateDSV(Texture, Desc, Flags == EDepthStencilViewFlags::None ? "DefaultDSV" : "ReadOnlyDSV");
    DefaultDepthStencilViews.Emplace(View);
    return View;
}

FRenderGraphShaderResourceView* FRenderGraphBuilder::GetOrCreateDefaultSRV(FRenderGraphBuffer* Buffer)
{
    for (FRenderGraphShaderResourceView* View : DefaultBufferShaderResourceViews)
    {
        if (View->GetParentBuffer() == Buffer)
        {
            return View;
        }
    }

    FRenderGraphShaderResourceView* View = CreateSRV(Buffer, RenderGraphDefaultViewDescs::ShaderResourceForBuffer(Buffer->GetDesc().BufferDesc), "DefaultBufferSRV");
    DefaultBufferShaderResourceViews.Emplace(View);
    return View;
}

FRenderGraphUnorderedAccessView* FRenderGraphBuilder::GetOrCreateDefaultUAV(FRenderGraphBuffer* Buffer)
{
    for (FRenderGraphUnorderedAccessView* View : DefaultBufferUnorderedAccessViews)
    {
        if (View->GetParentBuffer() == Buffer)
        {
            return View;
        }
    }

    FRenderGraphUnorderedAccessView* View = CreateUAV(Buffer, RenderGraphDefaultViewDescs::UnorderedAccessForBuffer(Buffer->GetDesc().BufferDesc), "DefaultBufferUAV");
    DefaultBufferUnorderedAccessViews.Emplace(View);
    return View;
}

FRenderGraphPass* FRenderGraphBuilder::AllocatePass(const CHAR* InName, ERenderGraphPassFlags InFlags, bool bEnabled)
{
    if (bIsCompiled)
    {
        LOG_ERROR("Graph '%s' cannot add pass '%s' after it has been compiled", Name, InName ? InName : "Unnamed");
        return nullptr;
    }

    if (!bEnabled)
    {
        ++Statistics.NumDisabledPasses;
    }

    void* PassMemory = Memory.Allocate(sizeof(FRenderGraphPass), alignof(FRenderGraphPass));

    FRenderGraphPass* Pass = new(PassMemory) FRenderGraphPass(InName, InFlags, bEnabled);
    Passes.Emplace(Pass);
    return Pass;
}

bool FRenderGraphBuilder::IsAccessPlannedThroughView(const FRenderGraphPass& Pass, FRenderGraphTexture* Texture, ERHIResourceState State, bool bIsWrite)
{
    ERHIResourceState CoveredReadStates   = ERHIResourceState::Common;
    bool              bFoundMatchingWrite = false;

    for (const FRenderGraphViewAccess& Access : Pass.GetViewAccesses())
    {
        if (!Access.bIsTextureParent || Access.ParentTexture != Texture)
        {
            continue;
        }

        if (Access.bIsWrite)
        {
            bFoundMatchingWrite |= (Access.State == State);
        }
        else
        {
            CoveredReadStates |= Access.State;
        }
    }

    return IsAccessPlannedThroughViews(CoveredReadStates, bFoundMatchingWrite, State, bIsWrite);
}

bool FRenderGraphBuilder::IsAccessPlannedThroughView(const FRenderGraphPass& Pass, FRenderGraphBuffer* Buffer, ERHIResourceState State, bool bIsWrite)
{
    ERHIResourceState CoveredReadStates   = ERHIResourceState::Common;
    bool              bFoundMatchingWrite = false;

    for (const FRenderGraphViewAccess& Access : Pass.GetViewAccesses())
    {
        if (Access.bIsTextureParent || Access.ParentBuffer != Buffer)
        {
            continue;
        }

        if (Access.bIsWrite)
        {
            bFoundMatchingWrite |= (Access.State == State);
        }
        else
        {
            CoveredReadStates |= Access.State;
        }
    }

    return IsAccessPlannedThroughViews(CoveredReadStates, bFoundMatchingWrite, State, bIsWrite);
}

void FRenderGraphBuilder::PlanTextureViewBarrier(FRenderGraphPass* Pass, FRHITexture* RHITexture, FRenderGraphResourceState& State, const FRHITextureSubresourceRange& Range, ERHIResourceState AccessState, bool bIsWrite)
{
    TransitionTexture(RHITexture, State, Range, AccessState, bIsWrite, Pass->GetTransitions(), Pass->GetUnorderedAccessBarriers(), Statistics);
}

void FRenderGraphBuilder::PlanBufferViewBarrier(FRenderGraphPass* Pass, FRHIBuffer* RHIBuffer, FRenderGraphResourceState& State, const FBufferRegion& Range, ERHIResourceState AccessState, bool bIsWrite)
{
    if (!RHIBuffer)
    {
        return;
    }

    if (RHIBuffer->GetDesc().TrackingMode == ERHIResourceStateTrackingMode::Static)
    {
        return;
    }

    const bool bIsSubrange = !Range.IsWholeResource();
    if (State.CurrentState == AccessState)
    {
        if (AccessState == ERHIResourceState::UnorderedAccess && State.bWrittenAsUnorderedAccess)
        {
            Pass->AddUnorderedAccessBarrier(FRHIUnorderedAccessBarrierDesc::CreateBufferRange(RHIBuffer, Range));
            ++Statistics.NumUnorderedAccessBarriers;
            Statistics.NumSubresourceBarriers += bIsSubrange ? 1 : 0;
        }
    }
    else
    {
        Pass->AddTransition(FRHITransitionBarrierDesc::CreateBufferRange(RHIBuffer, State.CurrentState, AccessState, Range));
        ++Statistics.NumTransitionBarriers;
        Statistics.NumSubresourceBarriers += bIsSubrange ? 1 : 0;
        State.CurrentState = AccessState;
    }

    State.bWrittenAsUnorderedAccess = bIsWrite && (AccessState == ERHIResourceState::UnorderedAccess);
}

bool FRenderGraphBuilder::HasLiveOutput(const FRenderGraphPass& Pass) const
{
    for (const FRenderGraphTextureAccess& Access : Pass.GetTextureAccesses())
    {
        if (Access.bIsWrite && Access.Resource->State.NumReaders > 0)
        {
            return true;
        }
    }

    for (const FRenderGraphBufferAccess& Access : Pass.GetBufferAccesses())
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
        if (!Pass->IsEnabled() || Pass->IsCulled())
        {
            continue;
        }

        for (const FRenderGraphTextureAccess& Access : Pass->GetTextureAccesses())
        {
            if (!Access.bIsWrite)
            {
                ++Access.Resource->State.NumReaders;
            }
        }

        for (const FRenderGraphBufferAccess& Access : Pass->GetBufferAccesses())
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
            if (Pass->IsCulled() || !Pass->IsEnabled() || IsEnumFlagSet(Pass->GetFlags(), ERenderGraphPassFlags::NeverCull))
            {
                continue;
            }

            if (HasLiveOutput(*Pass))
            {
                continue;
            }

            Pass->SetCulled(true);
            bCulledAnyPass = true;
            ++Statistics.NumCulledPasses;

            for (const FRenderGraphTextureAccess& Access : Pass->GetTextureAccesses())
            {
                if (!Access.bIsWrite)
                {
                    --Access.Resource->State.NumReaders;
                }
            }

            for (const FRenderGraphBufferAccess& Access : Pass->GetBufferAccesses())
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
        if (Pass->IsCulled() || !Pass->IsEnabled())
        {
            continue;
        }

        for (const FRenderGraphTextureAccess& Access : Pass->GetTextureAccesses())
        {
            FRenderGraphResourceState& State = Access.Resource->State;
            if (State.FirstPassIndex < 0)
            {
                State.FirstPassIndex = PassIndex;
            }

            State.LastPassIndex = PassIndex;
        }

        for (const FRenderGraphBufferAccess& Access : Pass->GetBufferAccesses())
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
        if (Texture->IsExternal() || Texture->State.FirstPassIndex < 0)
        {
            continue;
        }

        Texture->Texture = Pool.AcquireTexture(Texture->Desc, Texture->Name, Texture->State.CurrentState);
        if (Texture->Texture)
        {
            Texture->State.AcquiredState = Texture->State.CurrentState;
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
            Buffer->State.AcquiredState = Buffer->State.CurrentState;
            ++Statistics.NumBuffersAllocated;
        }
    }
}

void FRenderGraphBuilder::ValidateGraph()
{
    for (FRenderGraphPass* Pass : Passes)
    {
        if (Pass->IsCulled() || !Pass->IsEnabled())
        {
            continue;
        }

        if (Pass->HasConflictingAccesses())
        {
            LOG_ERROR("Graph '%s' cannot compile because pass '%s' declared conflicting accesses", Name, Pass->GetName());
            SetHasErrors(true);
        }

        RenderGraphViewValidation::ValidatePassViewAccesses(*this, *Pass);
    }
}

void FRenderGraphBuilder::PlanBarriers()
{
    for (FRenderGraphPass* Pass : Passes)
    {
        if (Pass->IsCulled() || !Pass->IsEnabled())
        {
            continue;
        }

        for (const FRenderGraphViewAccess& Access : Pass->GetViewAccesses())
        {
            if (Access.bIsTextureParent)
            {
                PlanTextureViewBarrier(Pass, Access.ParentTexture->GetRHITexture(), Access.ParentTexture->State, Access.SubresourceRange, Access.State, Access.bIsWrite);
            }
            else
            {
                PlanBufferViewBarrier(Pass, Access.ParentBuffer->GetRHIBuffer(), Access.ParentBuffer->State, Access.BufferRange, Access.State, Access.bIsWrite);
            }
        }

        for (const FRenderGraphTextureAccess& Access : Pass->GetTextureAccesses())
        {
            if (IsAccessPlannedThroughView(*Pass, Access.Resource, Access.State, Access.bIsWrite))
            {
                continue;
            }

            PlanTextureViewBarrier(Pass, Access.Resource->GetRHITexture(), Access.Resource->State, FRHITextureSubresourceRange::All(), Access.State, Access.bIsWrite);
        }

        for (const FRenderGraphBufferAccess& Access : Pass->GetBufferAccesses())
        {
            if (IsAccessPlannedThroughView(*Pass, Access.Resource, Access.State, Access.bIsWrite))
            {
                continue;
            }

            PlanBufferViewBarrier(Pass, Access.Resource->GetRHIBuffer(), Access.Resource->State, FBufferRegion::Whole(), Access.State, Access.bIsWrite);
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
    ValidateGraph();
    AllocateResources();
    PlanBarriers();
}

FRHIBeginRenderPassDesc FRenderGraphBuilder::BuildBeginRenderPassDesc(const FRenderGraphPass& Pass, FRenderGraphViewCache& ViewCache) const
{
    FRHIBeginRenderPassDesc::FRenderTargetAttachments RenderTargets;

    uint32 NumRenderTargets = 0;
    for (uint32 Index = 0; Index < Pass.GetNumRenderTargets(); ++Index)
    {
        const FRenderGraphAttachment& Attachment = Pass.GetRenderTargets()[Index];
        FRHIRenderTargetView* RenderTargetView = ResolveRenderTargetAttachmentView(Attachment, ViewCache);
        if (!RenderTargetView)
        {
            LOG_ERROR("Graph '%s' pass '%s' failed to resolve render-target attachment %u ('%s')", Name, Pass.GetName(), Index, Attachment.Texture ? Attachment.Texture->GetName() : "null");
            return FRHIBeginRenderPassDesc();
        }

        RenderTargets[Index] = FRHIRenderTargetAttachment(RenderTargetView, Attachment.LoadAction, Attachment.StoreAction, Attachment.ClearValue);
        NumRenderTargets     = Math::Max(NumRenderTargets, Index + 1);
    }

    FRHIDepthStencilAttachment DepthStencilAttachment;
    const FRenderGraphDepthAttachment& DepthStencil = Pass.GetDepthStencil();
    if (FRHIDepthStencilView* DepthStencilView = ResolveDepthStencilAttachmentView(DepthStencil, ViewCache))
    {
        DepthStencilAttachment = FRHIDepthStencilAttachment( DepthStencilView, DepthStencil.LoadAction, DepthStencil.StoreAction, DepthStencil.DepthStencilClearValue);
    }
    else if (DepthStencil.DepthStencilView || DepthStencil.Texture)
    {
        LOG_ERROR("Graph '%s' pass '%s' failed to resolve depth-stencil attachment ('%s')", Name, Pass.GetName(), DepthStencil.Texture ? DepthStencil.Texture->GetName() : "null");
    }

    return FRHIBeginRenderPassDesc(RenderTargets, NumRenderTargets, DepthStencilAttachment);
}

void FRenderGraphBuilder::EmitEpilogueBarriers(FRHICommandList& CommandList)
{
    TArray<FRHITransitionBarrierDesc>      Transitions;
    TArray<FRHIUnorderedAccessBarrierDesc> UnorderedAccessBarriers;

    for (FRenderGraphTexture* Texture : Textures)
    {
        FRenderGraphResourceState& State       = Texture->State;
        ERHIResourceState          TargetState = State.CurrentState;

        if (Texture->IsExternal())
        {
            TargetState = State.FinalState;
        }
        else if (State.bSubresourcesDiverged)
        {
            TargetState = State.SubresourceStates.First().State;
        }

        if (!State.bSubresourcesDiverged && State.CurrentState == TargetState)
        {
            continue;
        }

        TransitionTexture(Texture->GetRHITexture(), State, FRHITextureSubresourceRange::All(), TargetState, false, Transitions, UnorderedAccessBarriers, Statistics);
    }

    for (FRenderGraphBuffer* Buffer : Buffers)
    {
        if (!Buffer->IsExternal() || Buffer->State.CurrentState == Buffer->State.FinalState)
        {
            continue;
        }

        if (Buffer->GetRHIBuffer()->GetDesc().TrackingMode == ERHIResourceStateTrackingMode::Static)
        {
            continue;
        }

        Transitions.Emplace(FRHITransitionBarrierDesc::CreateBuffer(Buffer->GetRHIBuffer(), Buffer->State.CurrentState, Buffer->State.FinalState));

        Buffer->State.CurrentState = Buffer->State.FinalState;
        ++Statistics.NumTransitionBarriers;
    }

    if (!Transitions.IsEmpty())
    {
        CommandList.TransitionBarrier(MakeArrayView(Transitions));
    }

    if (!UnorderedAccessBarriers.IsEmpty())
    {
        CommandList.UnorderedAccessBarrier(MakeArrayView(UnorderedAccessBarriers));
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
            Pool.ReleaseTexture(Texture->Texture, bIsExecuted ? Texture->State.CurrentState : Texture->State.AcquiredState);
        }
    }

    for (FRenderGraphBuffer* Buffer : Buffers)
    {
        if (!Buffer->IsExternal())
        {
            Pool.ReleaseBuffer(Buffer->Buffer, bIsExecuted ? Buffer->State.CurrentState : Buffer->State.AcquiredState);
        }
    }
}

void FRenderGraphBuilder::Execute(FRHICommandList& CommandList)
{
    if (!bIsCompiled)
    {
        Compile();
    }

    if (bHasErrors)
    {
        LOG_ERROR("Graph '%s' cannot execute because compilation reported errors", Name);
        return;
    }

    if (bIsExecuted)
    {
        LOG_ERROR("Graph '%s' has already been executed. Build a new graph rather than replaying this one", Name);
        return;
    }

    bIsExecuted = true;

    FRenderGraphViewCache ViewCache;

    RHI_EVENT_SCOPE(CommandList, Name);

    for (FRenderGraphPass* Pass : Passes)
    {
        if (Pass->IsCulled() || !Pass->IsEnabled())
        {
            continue;
        }

        RHI_EVENT_SCOPE(CommandList, Pass->GetName());

        if (!Pass->GetTransitions().IsEmpty())
        {
            CommandList.TransitionBarrier(MakeArrayView(Pass->GetTransitions()));
        }

        if (!Pass->GetUnorderedAccessBarriers().IsEmpty())
        {
            CommandList.UnorderedAccessBarrier(MakeArrayView(Pass->GetUnorderedAccessBarriers()));
        }

        FRHIBeginRenderPassDesc RenderPassDesc;

        const bool bIsRaster      = Pass->IsRaster();
        const bool bHasRenderPass = bIsRaster && (RenderPassDesc = BuildBeginRenderPassDesc(*Pass, ViewCache), HasValidRenderPassAttachments(RenderPassDesc));

        if (bIsRaster && !bHasRenderPass)
        {
            LOG_ERROR("Graph '%s' pass '%s' is flagged Raster but has no resolvable attachments; skipping render pass", Name, Pass->GetName());
        }

        if (bHasRenderPass)
        {
            CommandList.BeginRenderPass(RenderPassDesc);
        }

        if (FRenderGraphPassExecutor* Executor = Pass->GetExecutor())
        {
            FRenderGraphPassResources Resources = FRenderGraphPassResources::Create(*Pass, ViewCache);
            Executor->Execute(CommandList, Resources);
        }

        if (bHasRenderPass)
        {
            CommandList.EndRenderPass();
        }
    }

    Statistics.NumViewsCreated   = ViewCache.GetNumViewsCreated();
    Statistics.NumViewsCacheHits = ViewCache.GetNumViewsCacheHits();

    EmitEpilogueBarriers(CommandList);

    ReleasePooledResources();

    if (FRenderGraphResourcePool::IsInitialized())
    {
        FRenderGraphResourcePool::Get().Tick();
    }
}
