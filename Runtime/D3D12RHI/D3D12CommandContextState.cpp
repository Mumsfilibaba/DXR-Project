#include "Core/Memory/Memory.h"
#include "D3D12RHI/D3D12CommandContextState.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/RayTracing/D3D12RayTracingPipeline.h"

#if D3D12_ENABLE_DESCRIPTOR_HEAP_ROLLOVER_LOGGING
static void LogDescriptorHeapRollover(const String& PSOName, bool bCommandListSplit, uint32 NumResourceDescriptors)
{
    const CHAR* PSONameStr = PSOName.Length() > 0 ? PSOName.Data() : "<unnamed>";
    D3D12_INFO("[DescriptorHeapRollover] %s (PSO: '%s', RequestedDescriptors: %u)",
        bCommandListSplit ? "CommandList split" : "Realloc-only rollover", PSONameStr, NumResourceDescriptors);
}
#endif

static D3D12_RESOURCE_STATES D3D12GetShaderStageReadState(ED3D12CommandQueueType QueueType, EShaderVisibility::Type ShaderStage)
{
    const D3D12_RESOURCE_STATES StageState = (ShaderStage == EShaderVisibility::Pixel)
        ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    const D3D12_RESOURCE_STATES AllowedState = StageState & D3D12GetAllowedReadStates(QueueType);
    return AllowedState ? AllowedState : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
}

FD3D12CommandContextState::FD3D12CommandContextState(FD3D12Device* InDevice, FD3D12CommandContext& InContext)
    : FD3D12DeviceChild(InDevice)
    , Context(InContext)
    , GraphicsState()
    , ComputeState()
    , MeshletState()
    , CommonState(InDevice, InContext)
{
}

FD3D12CommandContextState::~FD3D12CommandContextState() = default;

bool FD3D12CommandContextState::Initialize()
{
    if (!CommonState.DescriptorCache.Initialize())
    {
        D3D12_ERROR_CRITICAL("Failed to initialize DescriptorCache");
        return false;
    }

    ResetState();
    return true;
}

void FD3D12CommandContextState::PrepareGraphicsState()
{
    ActivePipeline = EActivePipeline::Graphics;

    bool bCommandListSplit;
    FD3D12RootSignature* RootSignature = GraphicsState.PipelineState->GetRootSignature();
    do
    {
        bCommandListSplit = false;
        bCommandListSplit |= PrepareResources(RootSignature, GraphicsState.PipelineState.Get(), EShaderVisibility::Vertex, EShaderVisibility::Pixel);
        bCommandListSplit |= PrepareSamplers(RootSignature, GraphicsState.PipelineState.Get(), EShaderVisibility::Vertex, EShaderVisibility::Pixel);
    } while (bCommandListSplit);

    FD3D12RenderTargetCache& RenderTargetCache = CommonGraphicsState.RenderTargetCache;
    for (uint32 i = 0; i < RenderTargetCache.NumRenderTargets; i++)
    {
        if (FD3D12RenderTargetViewRHI* RenderTargetView = RenderTargetCache.RenderTargetViews[i])
        {
            Context.TransitionResourceState(RenderTargetView);
        }
    }

    if (FD3D12DepthStencilViewRHI* DepthStencilView = RenderTargetCache.DepthStencilView)
    {
        D3D12_RESOURCE_STATES DesiredState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (DepthStencilView->IsReadOnly())
        {
            // PrepareResources ran first and already transitioned this texture for its SRV binds. Asking for
            // DEPTH_READ on its own here would undo that and leave the shader reads without a barrier, so the
            // accumulated read state is folded in. DEPTH_READ combines with the shader-read states.
            DesiredState = D3D12_RESOURCE_STATE_DEPTH_READ | GetAccumulatedSRVReadState(DepthStencilView->GetViewResource(), D3D12_RESOURCE_STATE_DEPTH_READ);
        }

        Context.TransitionResourceState(DepthStencilView, DesiredState);
    }

    TransitionVertexAndIndexBuffers();

#if D3D12_USE_ID3D12COMMANDLIST_5
    Context.TransitionTrackedResourceState(CommonGraphicsState.ShadingRateImage, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
#endif
}

void FD3D12CommandContextState::TransitionVertexAndIndexBuffers()
{
    for (uint32 i = 0; i < GraphicsState.VertexBufferCache.NumVertexBuffers; i++)
    {
        Context.TransitionTrackedResourceState(GraphicsState.VertexBufferCache.BufferResources[i], D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    }

    Context.TransitionTrackedResourceState(GraphicsState.IndexBufferCache.BufferResource, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    for (uint32 i = 0; i < GraphicsState.NumSOBuffers; i++)
    {
        Context.TransitionTrackedResourceState(GraphicsState.SOBuffers[i], D3D12_RESOURCE_STATE_STREAM_OUT);
    }
}

void FD3D12CommandContextState::BindGraphicsState()
{
    FD3D12RootSignature* RootSignature = GraphicsState.PipelineState->GetRootSignature();

    if (GraphicsState.bBindPipelineState)
    {
        Context.GetCommandList()->SetPipelineState(GraphicsState.PipelineState->GetD3D12PipelineState());
        GraphicsState.bBindPipelineState = false;
    }

    if (GraphicsState.bBindPrimitiveTopology)
    {
        Context.GetCommandList()->IASetPrimitiveTopology(GraphicsState.PipelineState->GetD3D12PrimitiveTopology());
        GraphicsState.bBindPrimitiveTopology = false;
    }

    // -----------------------------------------------------------------------------------------------------------
    // D3D12 Spec: when a root signature carries D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
    // (or its sampler counterpart) the shader-visible heaps must already be bound on the command list before 
    // SetGraphicsRootSignature is called. SetDescriptorHeaps is idempotent (skips when unchanged), so the 
    // redundant call at the top of BindResources stays safe.
    // -----------------------------------------------------------------------------------------------------------

    CommonState.DescriptorCache.SetDescriptorHeaps();

    InternalSetRootSignature(RootSignature, /*bIsCompute=*/false);

    if (CommonGraphicsState.bBindRenderTargets)
    {
        CommonState.DescriptorCache.SetRenderTargets(CommonGraphicsState.RenderTargetCache);
        CommonGraphicsState.bBindRenderTargets = false;
    }

#if D3D12_USE_ID3D12COMMANDLIST_5
    if (Context.GetCommandList().GetGraphicsCommandList5().IsValid())
    {
        if (CommonGraphicsState.bBindShadingRateImage)
        {
            ID3D12Resource* Resource = CommonGraphicsState.ShadingRateImage ? CommonGraphicsState.ShadingRateImage->GetResource()->GetD3D12Resource() : nullptr;
            Context.GetCommandList().GetGraphicsCommandList5()->RSSetShadingRateImage(Resource);
            CommonGraphicsState.bBindShadingRateImage = false;

            if (CommonGraphicsState.ShadingRateImage)
            {
                Context.GetCommandList().UpdateResidency(CommonGraphicsState.ShadingRateImage->GetResource()->GetResidencyHandle());
            }
        }

        if (CommonGraphicsState.bBindShadingRate)
        {
            D3D12_SHADING_RATE_COMBINER Combiners[] =
            {
                D3D12_SHADING_RATE_COMBINER_OVERRIDE,
                D3D12_SHADING_RATE_COMBINER_OVERRIDE,
            };

            Context.GetCommandList().GetGraphicsCommandList5()->RSSetShadingRate(CommonGraphicsState.ShadingRate, Combiners);
            CommonGraphicsState.bBindShadingRate = false;
        }
    }
#endif

    BindResources(RootSignature, EShaderVisibility::Vertex, EShaderVisibility::Pixel);
    BindSamplers(RootSignature, EShaderVisibility::Vertex, EShaderVisibility::Pixel);

    if (GraphicsState.bBindShaderConstants)
    {
        BindShaderConstants(RootSignature, EShaderConstantsPipeline::Graphics);
        GraphicsState.bBindShaderConstants = false;
    }

    if (GraphicsState.bBindVertexBuffers)
    {
        CommonState.DescriptorCache.SetVertexBuffers(GraphicsState.VertexBufferCache);
        GraphicsState.bBindVertexBuffers = false;
    }

    if (GraphicsState.bBindIndexBuffer)
    {
        CommonState.DescriptorCache.SetIndexBuffer(GraphicsState.IndexBufferCache);
        GraphicsState.bBindIndexBuffer = false;
    }

    if (CommonGraphicsState.bBindViewports)
    {
        Context.GetCommandList()->RSSetViewports(CommonGraphicsState.NumViewports, CommonGraphicsState.Viewports);
        CommonGraphicsState.bBindViewports = false;
    }

    if (CommonGraphicsState.bBindScissorRects)
    {
        Context.GetCommandList()->RSSetScissorRects(CommonGraphicsState.NumScissorRects, CommonGraphicsState.ScissorRects);
        CommonGraphicsState.bBindScissorRects = false;
    }

    if (CommonGraphicsState.bBindBlendFactor)
    {
        Context.GetCommandList()->OMSetBlendFactor(CommonGraphicsState.BlendFactor);
        CommonGraphicsState.bBindBlendFactor = false;
    }

    if (CommonGraphicsState.bBindStencilRef)
    {
        FD3D12GraphicsPipelineStateRHI* BoundPSO = GraphicsState.PipelineState.Get();
        
        const bool bShaderOverridesStencilRef = (BoundPSO != nullptr) && IsEnumFlagSet(BoundPSO->GetShaderFlags(), ED3D12ShaderFlags::RequiresStencilRef);
        if (!bShaderOverridesStencilRef)
        {
            Context.GetCommandList()->OMSetStencilRef(CommonGraphicsState.StencilRef);
        }

        CommonGraphicsState.bBindStencilRef = false;
    }

    FlushDepthBias();
    FlushDepthBounds();
    FlushSamplePositions();

    if (GraphicsState.bBindStreamOutputTargets)
    {
        for (uint32 i = 0; i < GraphicsState.NumSOBuffers; i++)
        {
            if (FD3D12BufferRHI* Buffer = GraphicsState.SOBuffers[i])
            {
                Context.GetCommandList().UpdateResidency(Buffer->GetResource()->GetResidencyHandle());
            }
        }

        Context.GetCommandList()->SOSetTargets(0, GraphicsState.NumSOBuffers, GraphicsState.SOBufferViews);
        GraphicsState.bBindStreamOutputTargets = false;
    }
}

void FD3D12CommandContextState::PrepareComputeState()
{
    ActivePipeline = EActivePipeline::Compute;

    bool bCommandListSplit;
    FD3D12RootSignature* RootSignature = ComputeState.PipelineState->GetRootSignature();
    do
    {
        bCommandListSplit = false;
        bCommandListSplit |= PrepareResources(RootSignature, ComputeState.PipelineState.Get(), EShaderVisibility::All, EShaderVisibility::All);
        bCommandListSplit |= PrepareSamplers(RootSignature, ComputeState.PipelineState.Get(), EShaderVisibility::All, EShaderVisibility::All);
    } while (bCommandListSplit);
}

void FD3D12CommandContextState::BindComputeState()
{
    FD3D12RootSignature* RootSignature = ComputeState.PipelineState->GetRootSignature();

#if D3D12_ENABLE_PIPELINE_BIND_LOGGING
    {
        String PSODebugName;
        ComputeState.PipelineState->GetDebugName(PSODebugName);
        
        const CHAR* PSONameStr = PSODebugName.Length() > 0 ? PSODebugName.Data() : "<unnamed>";
        D3D12_INFO("[PSO-BIND] Compute dispatch PSO=%p ('%s')", reinterpret_cast<void*>(ComputeState.PipelineState.Get()), PSONameStr);
    }
#endif

    if (ComputeState.bBindPipelineState)
    {
        Context.GetCommandList()->SetPipelineState(ComputeState.PipelineState->GetD3D12PipelineState());
        ComputeState.bBindPipelineState = false;
    }

    // -----------------------------------------------------------------------------------------------------------
    // See BindGraphicsState: shader-visible heaps must precede a directly-indexed root signature on the 
    // command list. SetDescriptorHeaps is idempotent so the BindResources call below stays safe.
    // -----------------------------------------------------------------------------------------------------------

    CommonState.DescriptorCache.SetDescriptorHeaps();

    InternalSetRootSignature(RootSignature, /*bIsCompute=*/true);

    BindResources(RootSignature, EShaderVisibility::All, EShaderVisibility::All);
    BindSamplers(RootSignature, EShaderVisibility::All, EShaderVisibility::All);

    if (ComputeState.bBindShaderConstants)
    {
        BindShaderConstants(RootSignature, EShaderConstantsPipeline::Compute);
        ComputeState.bBindShaderConstants = false;
    }
}

void FD3D12CommandContextState::BindRayTracingState()
{
    FD3D12RayTracingPipelineStateRHI* PipelineState = RayTracingState.PipelineState.Get();
    if (!PipelineState)
    {
        return;
    }

    FD3D12RootSignature* GlobalRootSignature = PipelineState->GetGlobalRootSignature();
    if (!GlobalRootSignature)
    {
        return;
    }

    ActivePipeline = EActivePipeline::RayTracing;

    bool bCommandListSplit;
    do
    {
        bCommandListSplit = false;
        bCommandListSplit |= PrepareResources(GlobalRootSignature, PipelineState, EShaderVisibility::All, EShaderVisibility::All);
        bCommandListSplit |= PrepareSamplers(GlobalRootSignature, PipelineState, EShaderVisibility::All, EShaderVisibility::All);
    } while (bCommandListSplit);

    // -----------------------------------------------------------------------------------------------------------
    // See BindGraphicsState: shader-visible heaps must precede a directly-indexed root signature on the 
    // command list. SetDescriptorHeaps is idempotent so the BindResources call below stays safe.
    // -----------------------------------------------------------------------------------------------------------
    
    CommonState.DescriptorCache.SetDescriptorHeaps();

    InternalSetRootSignature(GlobalRootSignature, /*bIsCompute=*/true);

    BindResources(GlobalRootSignature, EShaderVisibility::All, EShaderVisibility::All);
    BindSamplers(GlobalRootSignature, EShaderVisibility::All, EShaderVisibility::All);

    if (RayTracingState.bBindShaderConstants)
    {
        BindShaderConstants(GlobalRootSignature, EShaderConstantsPipeline::RayTracing);
        RayTracingState.bBindShaderConstants = false;
    }

    ComputeState.bBindPipelineState = true;
}

void FD3D12CommandContextState::PrepareMeshletState()
{
    ActivePipeline = EActivePipeline::Meshlet;

    bool bCommandListSplit;
    FD3D12RootSignature* RootSignature = MeshletState.PipelineState->GetRootSignature();
    do
    {
        bCommandListSplit = false;
        bCommandListSplit |= PrepareResources(RootSignature, MeshletState.PipelineState.Get(), EShaderVisibility::Pixel, EShaderVisibility::Mesh);
        bCommandListSplit |= PrepareSamplers(RootSignature, MeshletState.PipelineState.Get(), EShaderVisibility::Pixel, EShaderVisibility::Mesh);
    } while (bCommandListSplit);

    FD3D12RenderTargetCache& RenderTargetCache = CommonGraphicsState.RenderTargetCache;
    for (uint32 i = 0; i < RenderTargetCache.NumRenderTargets; i++)
    {
        if (FD3D12RenderTargetViewRHI* RenderTargetView = RenderTargetCache.RenderTargetViews[i])
        {
            Context.TransitionResourceState(RenderTargetView);
        }
    }

    if (FD3D12DepthStencilViewRHI* DepthStencilView = RenderTargetCache.DepthStencilView)
    {
        D3D12_RESOURCE_STATES DesiredState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (DepthStencilView->IsReadOnly())
        {
            // PrepareResources ran first and already transitioned this texture for its SRV binds. Asking for
            // DEPTH_READ on its own here would undo that and leave the shader reads without a barrier, so the
            // accumulated read state is folded in; DEPTH_READ combines with the shader-read states.
            DesiredState = D3D12_RESOURCE_STATE_DEPTH_READ | GetAccumulatedSRVReadState(DepthStencilView->GetViewResource(), D3D12_RESOURCE_STATE_DEPTH_READ);
        }

        Context.TransitionResourceState(DepthStencilView, DesiredState);
    }

    TransitionVertexAndIndexBuffers();

#if D3D12_USE_ID3D12COMMANDLIST_5
    Context.TransitionTrackedResourceState(CommonGraphicsState.ShadingRateImage, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
#endif
}

void FD3D12CommandContextState::BindMeshletState()
{
    FD3D12RootSignature* RootSignature = MeshletState.PipelineState->GetRootSignature();

    if (MeshletState.bBindPipelineState)
    {
        Context.GetCommandList()->SetPipelineState(MeshletState.PipelineState->GetD3D12PipelineState());
        MeshletState.bBindPipelineState = false;
    }

    // See BindGraphicsState: shader-visible heaps must precede a directly-indexed root signature.
    CommonState.DescriptorCache.SetDescriptorHeaps();

    InternalSetRootSignature(RootSignature, /*bIsCompute=*/false);

    if (CommonGraphicsState.bBindRenderTargets)
    {
        CommonState.DescriptorCache.SetRenderTargets(CommonGraphicsState.RenderTargetCache);
        CommonGraphicsState.bBindRenderTargets = false;
    }

#if D3D12_USE_ID3D12COMMANDLIST_5
    if (Context.GetCommandList().GetGraphicsCommandList5().IsValid())
    {
        if (CommonGraphicsState.bBindShadingRateImage)
        {
            ID3D12Resource* Resource = CommonGraphicsState.ShadingRateImage ? CommonGraphicsState.ShadingRateImage->GetResource()->GetD3D12Resource() : nullptr;
            Context.GetCommandList().GetGraphicsCommandList5()->RSSetShadingRateImage(Resource);
            CommonGraphicsState.bBindShadingRateImage = false;

            if (CommonGraphicsState.ShadingRateImage)
            {
                Context.GetCommandList().UpdateResidency(CommonGraphicsState.ShadingRateImage->GetResource()->GetResidencyHandle());
            }
        }

        if (CommonGraphicsState.bBindShadingRate)
        {
            D3D12_SHADING_RATE_COMBINER Combiners[] =
            {
                D3D12_SHADING_RATE_COMBINER_OVERRIDE,
                D3D12_SHADING_RATE_COMBINER_OVERRIDE,
            };

            Context.GetCommandList().GetGraphicsCommandList5()->RSSetShadingRate(CommonGraphicsState.ShadingRate, Combiners);
            CommonGraphicsState.bBindShadingRate = false;
        }
    }
#endif

    BindResources(RootSignature, EShaderVisibility::Pixel, EShaderVisibility::Mesh);
    BindSamplers(RootSignature, EShaderVisibility::Pixel, EShaderVisibility::Mesh);

    if (MeshletState.bBindShaderConstants)
    {
        BindShaderConstants(RootSignature, EShaderConstantsPipeline::Graphics);
        MeshletState.bBindShaderConstants = false;
    }

    if (CommonGraphicsState.bBindViewports)
    {
        Context.GetCommandList()->RSSetViewports(CommonGraphicsState.NumViewports, CommonGraphicsState.Viewports);
        CommonGraphicsState.bBindViewports = false;
    }

    if (CommonGraphicsState.bBindScissorRects)
    {
        Context.GetCommandList()->RSSetScissorRects(CommonGraphicsState.NumScissorRects, CommonGraphicsState.ScissorRects);
        CommonGraphicsState.bBindScissorRects = false;
    }

    if (CommonGraphicsState.bBindBlendFactor)
    {
        Context.GetCommandList()->OMSetBlendFactor(CommonGraphicsState.BlendFactor);
        CommonGraphicsState.bBindBlendFactor = false;
    }

    if (CommonGraphicsState.bBindStencilRef)
    {
        FD3D12MeshletPipelineStateRHI* BoundPSO = MeshletState.PipelineState.Get();

        const bool bShaderOverridesStencilRef = (BoundPSO != nullptr) && IsEnumFlagSet(BoundPSO->GetShaderFlags(), ED3D12ShaderFlags::RequiresStencilRef);
        if (!bShaderOverridesStencilRef)
        {
            Context.GetCommandList()->OMSetStencilRef(CommonGraphicsState.StencilRef);
        }

        CommonGraphicsState.bBindStencilRef = false;
    }

    FlushDepthBias();
    FlushDepthBounds();
    FlushSamplePositions();
}

bool FD3D12CommandContextState::PrepareSamplers(FD3D12RootSignature* RootSignature, const FD3D12EffectiveDescriptorCounts* PipelineState, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage)
{
    uint32 NumSamplers[EShaderVisibility::Count];

    constexpr int32 MaxTries = 4;
    bool bCommandListSplit = false;

    uint32 NumSamplerDescriptors;
    for (int32 NumTries = 0; NumTries < MaxTries; NumTries++)
    {
        NumSamplerDescriptors = 0;

        for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
        {
            const uint32 MaxSamplers = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::Sampler);
        #if D3D12_ENABLE_STATIC_DESCRIPTORS && D3D12_USE_VERSIONED_ROOT_SIGNATURES
            NumSamplers[CurrentStage] = MaxSamplers;
        #else
            if (PipelineState && GD3D12ResourceBindingTier > D3D12_RESOURCE_BINDING_TIER_1)
            {
                NumSamplers[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::Sampler);
            }
            else
            {
                NumSamplers[CurrentStage] = MaxSamplers;
            }
        #endif

            NumSamplerDescriptors += NumSamplers[CurrentStage];
        }

        if (!CommonState.DescriptorCache.GetSamplerHeap().HasSpace(NumSamplerDescriptors))
        {
            if (!CommonState.DescriptorCache.GetSamplerHeap().Realloc())
            {
                Context.SplitCommandListForDescriptorHeapRollover();
                bCommandListSplit = true;

                if (!CommonState.DescriptorCache.GetSamplerHeap().Realloc())
                {
                    if (!GetDevice()->ReallocateGlobalDescriptorHeap(ED3D12GlobalDescriptorHeapType::Sampler) || 
                        !CommonState.DescriptorCache.GetSamplerHeap().Realloc())
                    {
                        D3D12_ERROR("Failed to allocate sampler descriptor block after CommandList split + heap reallocate");
                        return bCommandListSplit;
                    }
                }
            }

            CommonState.DescriptorCache.InvalidateCachedSamplerTables();
            CommonState.SamplerStateCache.DirtyResourcesAll();
            continue;
        }

        break;
    }

    const uint32 StartHandleOffset = CommonState.DescriptorCache.GetSamplerHeap().AllocateHandles(NumSamplerDescriptors);
    uint32 DescriptorHandleOffset  = StartHandleOffset;

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        if (!NumSamplers[CurrentStage])
        {
            continue;
        }

        if (CommonState.SamplerStateCache.IsResourcesDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.PrepareSamplers(CommonState.SamplerStateCache, RootSignature, CurrentStage, NumSamplers[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumSamplerDescriptors);
        }
    }

    CommonState.DescriptorCache.GetSamplerHeap().SetCurrentHandle(DescriptorHandleOffset);
    return bCommandListSplit;
}

void FD3D12CommandContextState::AccumulateSRVReadStates(FD3D12RootSignature* RootSignature, const uint32* NumSRVs, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage)
{
    SRVReadStates.Clear();

    int32 NumContributingStages = 0;
    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        if (NumSRVs[CurrentStage] > 0)
        {
            NumContributingStages++;
        }
    }

    const FD3D12DepthStencilViewRHI* DepthStencilView = ((ActivePipeline == EActivePipeline::Graphics) || (ActivePipeline == EActivePipeline::Meshlet))
        ? CommonGraphicsState.RenderTargetCache.DepthStencilView
        : nullptr;

    const bool bHasReadOnlyDepth = DepthStencilView && DepthStencilView->IsReadOnly();

    if (NumContributingStages < 2 && !bHasReadOnlyDepth)
    {
        return;
    }

    const ED3D12CommandQueueType QueueType = Context.GetQueueType();
    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        if (NumSRVs[CurrentStage] == 0)
        {
            continue;
        }

        const D3D12_RESOURCE_STATES StageState = D3D12GetShaderStageReadState(QueueType, CurrentStage);

        const FD3D12DescriptorTableMapping& SRVMapping = RootSignature->GetDescriptorTableMapping(CurrentStage, EResourceType::SRV);
        auto& SRVCache = CommonState.ShaderResourceViewCache.ResourceViews[CurrentStage];

        for (uint32 Slot = 0; Slot < NumSRVs[CurrentStage]; Slot++)
        {
            const uint16 Register = SRVMapping.GetRegisterForSlot(static_cast<uint8>(Slot));

            FD3D12ShaderResourceViewRHI* View = SRVCache[Register];
            if (!View)
            {
                continue;
            }

            FD3D12Resource* Resource = View->GetViewResource();
            if (!Resource)
            {
                continue;
            }

            FAccumulatedReadState* Existing = nullptr;
            for (FAccumulatedReadState& ReadState : SRVReadStates)
            {
                if (ReadState.Resource == Resource)
                {
                    Existing = &ReadState;
                    break;
                }
            }

            if (Existing)
            {
                Existing->State |= StageState;
            }
            else
            {
                SRVReadStates.Emplace(FAccumulatedReadState{ Resource, StageState });
            }
        }
    }
}

D3D12_RESOURCE_STATES FD3D12CommandContextState::GetAccumulatedSRVReadState(FD3D12Resource* Resource, D3D12_RESOURCE_STATES StageState) const
{
    for (const FAccumulatedReadState& ReadState : SRVReadStates)
    {
        if (ReadState.Resource == Resource)
        {
            return ReadState.State;
        }
    }

    return StageState;
}

bool FD3D12CommandContextState::PrepareResources(FD3D12RootSignature* RootSignature, const FD3D12EffectiveDescriptorCounts* PipelineState, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage)
{
    uint32 NumCBVs[EShaderVisibility::Count];
    uint32 NumSRVs[EShaderVisibility::Count];
    uint32 NumUAVs[EShaderVisibility::Count];

    constexpr int32 MaxTries = 4;
    bool bCommandListSplit = false;

    uint32 NumResourceDescriptors;
    for (int32 NumTries = 0; NumTries < MaxTries; NumTries++)
    {
        NumResourceDescriptors = 0;

        for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
        {
            const uint32 MaxCBVs = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::CBV);
            const uint32 MaxSRVs = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::SRV);
            const uint32 MaxUAVs = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::UAV);

        #if D3D12_ENABLE_STATIC_DESCRIPTORS && D3D12_USE_VERSIONED_ROOT_SIGNATURES
            NumCBVs[CurrentStage] = MaxCBVs;
            NumSRVs[CurrentStage] = MaxSRVs;
            NumUAVs[CurrentStage] = MaxUAVs;
        #else
            if (PipelineState && GD3D12ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3)
            {
                NumCBVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::CBV);
                NumSRVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::SRV);
                NumUAVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::UAV);
            }
            else if (PipelineState && GD3D12ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2)
            {
                NumCBVs[CurrentStage] = MaxCBVs;
                NumSRVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::SRV);
                NumUAVs[CurrentStage] = MaxUAVs;
            }
            else
            {
                NumCBVs[CurrentStage] = MaxCBVs;
                NumSRVs[CurrentStage] = MaxSRVs;
                NumUAVs[CurrentStage] = MaxUAVs;
            }
        #endif

            NumResourceDescriptors += NumCBVs[CurrentStage];
            NumResourceDescriptors += NumSRVs[CurrentStage];
            NumResourceDescriptors += NumUAVs[CurrentStage];
        }

        bool bDescriptorHeapRolledOver = false;
        if (!CommonState.DescriptorCache.GetResourceHeap().HasSpace(NumResourceDescriptors))
        {
            if (!CommonState.DescriptorCache.GetResourceHeap().Realloc())
            {
                Context.SplitCommandListForDescriptorHeapRollover();
                bCommandListSplit = true;

                if (!CommonState.DescriptorCache.GetResourceHeap().Realloc())
                {
                    if (!GetDevice()->ReallocateGlobalDescriptorHeap(ED3D12GlobalDescriptorHeapType::Resource) ||
                        !CommonState.DescriptorCache.GetResourceHeap().Realloc())
                    {
                        D3D12_ERROR("Failed to allocate resource descriptor block after CommandList split + heap reallocate");
                        return bCommandListSplit;
                    }
                }
            }

            bDescriptorHeapRolledOver = true;
        }

        if (bDescriptorHeapRolledOver)
        {
        #if D3D12_ENABLE_DESCRIPTOR_HEAP_ROLLOVER_LOGGING
            {
                String PSODebugName;
                switch (ActivePipeline)
                {
                    case EActivePipeline::Graphics:
                    {
                        if (GraphicsState.PipelineState)
                        {
                            GraphicsState.PipelineState->GetDebugName(PSODebugName);
                        }

                        break;
                    }
                    case EActivePipeline::Compute:
                    {
                        if (ComputeState.PipelineState)
                        {
                            ComputeState.PipelineState->GetDebugName(PSODebugName);
                        }

                        break;
                    }
                    case EActivePipeline::Meshlet:
                    {
                        if (MeshletState.PipelineState)
                        {
                            MeshletState.PipelineState->GetDebugName(PSODebugName);
                        }

                        break;
                    }
                    case EActivePipeline::RayTracing:
                    {
                        if (RayTracingState.PipelineState)
                        {
                            RayTracingState.PipelineState->GetDebugName(PSODebugName);
                        }
                        
                        break;
                    }
                }

                LogDescriptorHeapRollover(PSODebugName, bCommandListSplit, NumResourceDescriptors);
            }
        #endif
            ResetStateResources();
            continue;
        }

        break;
    }

    AccumulateSRVReadStates(RootSignature, NumSRVs, StartStage, EndStage);

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        const D3D12_RESOURCE_STATES SRVState = D3D12GetShaderStageReadState(Context.GetQueueType(), CurrentStage);

        if (NumCBVs[CurrentStage] > 0)
        {
            const FD3D12DescriptorTableMapping& CBVMapping = RootSignature->GetDescriptorTableMapping(CurrentStage, EResourceType::CBV);
            auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[CurrentStage];
            
            bool bAlreadyDirty = CommonState.ConstantBufferCache.IsResourcesDirty(CurrentStage);
            for (uint32 Slot = 0; Slot < NumCBVs[CurrentStage]; Slot++)
            {
                const uint16 Register = CBVMapping.GetRegisterForSlot(static_cast<uint8>(Slot));
                if (FD3D12BufferRHI* Buffer = CBVCache[Register])
                {
                    Context.TransitionTrackedResourceState(Buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

                    if (!bAlreadyDirty)
                    {
                        if (FD3D12ConstantBufferView* View = Buffer->GetOrCreateConstantBufferView())
                        {
                            if (View->GetDescriptorVersion() != CommonState.ConstantBufferCache.ViewVersions[CurrentStage][Register])
                            {
                                CommonState.ConstantBufferCache.ViewVersions[CurrentStage][Register] = View->GetDescriptorVersion();
                                CommonState.ConstantBufferCache.DirtyResources(CurrentStage);
                                bAlreadyDirty = true;
                            }
                        }
                    }
                }
            }
        }

        {
            const FD3D12ShaderStage& Stage = RootSignature->GetShaderStage(CurrentStage);
            auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[CurrentStage];

            bool bAlreadyDirty = CommonState.ConstantBufferCache.IsResourcesDirty(CurrentStage);
            for (uint8 RootCBVIdx = 0; RootCBVIdx < Stage.GetNumRootCBVs() && !bAlreadyDirty; RootCBVIdx++)
            {
                const uint16 Register = Stage.GetRootCBVRegister(RootCBVIdx);
                if (FD3D12BufferRHI* Buffer = CBVCache[Register])
                {
                    Context.TransitionTrackedResourceState(Buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

                    if (FD3D12ConstantBufferView* View = Buffer->GetOrCreateConstantBufferView())
                    {
                        if (View->GetDescriptorVersion() != CommonState.ConstantBufferCache.ViewVersions[CurrentStage][Register])
                        {
                            CommonState.ConstantBufferCache.ViewVersions[CurrentStage][Register] = View->GetDescriptorVersion();
                            CommonState.ConstantBufferCache.DirtyResources(CurrentStage);
                            bAlreadyDirty = true;
                        }
                    }
                }
            }
        }

        if (NumSRVs[CurrentStage] > 0)
        {
            const FD3D12DescriptorTableMapping& SRVMapping = RootSignature->GetDescriptorTableMapping(CurrentStage, EResourceType::SRV);
            auto& SRVCache = CommonState.ShaderResourceViewCache.ResourceViews[CurrentStage];

            bool bAlreadyDirty = CommonState.ShaderResourceViewCache.IsResourcesDirty(CurrentStage);
            for (uint32 Slot = 0; Slot < NumSRVs[CurrentStage]; Slot++)
            {
                const uint16 Register = SRVMapping.GetRegisterForSlot(static_cast<uint8>(Slot));
                if (FD3D12ShaderResourceViewRHI* View = SRVCache[Register])
                {
                    Context.TransitionResourceState(View, GetAccumulatedSRVReadState(View->GetViewResource(), SRVState));

                    if (!bAlreadyDirty)
                    {
                        if (View->GetDescriptorVersion() != CommonState.ShaderResourceViewCache.ViewVersions[CurrentStage][Register])
                        {
                            CommonState.ShaderResourceViewCache.ViewVersions[CurrentStage][Register] = View->GetDescriptorVersion();
                            CommonState.ShaderResourceViewCache.DirtyResources(CurrentStage);
                            bAlreadyDirty = true;
                        }
                    }
                }
            }
        }

        if (NumUAVs[CurrentStage] > 0)
        {
            const FD3D12DescriptorTableMapping& UAVMapping = RootSignature->GetDescriptorTableMapping(CurrentStage, EResourceType::UAV);
            auto& UAVCache = CommonState.UnorderedAccessViewCache.ResourceViews[CurrentStage];

            bool bAlreadyDirty = CommonState.UnorderedAccessViewCache.IsResourcesDirty(CurrentStage);
            for (uint32 Slot = 0; Slot < NumUAVs[CurrentStage]; Slot++)
            {
                const uint16 Register = UAVMapping.GetRegisterForSlot(static_cast<uint8>(Slot));
                if (FD3D12UnorderedAccessViewRHI* View = UAVCache[Register])
                {
                    Context.TransitionResourceState(View);

                    if (!bAlreadyDirty)
                    {
                        if (View->GetDescriptorVersion() != CommonState.UnorderedAccessViewCache.ViewVersions[CurrentStage][Register])
                        {
                            CommonState.UnorderedAccessViewCache.ViewVersions[CurrentStage][Register] = View->GetDescriptorVersion();
                            CommonState.UnorderedAccessViewCache.DirtyResources(CurrentStage);
                            bAlreadyDirty = true;
                        }
                    }
                }
            }
        }
    }

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        if (NumCBVs[CurrentStage] > 0 && CommonState.DescriptorCache.IsTableLayoutStale(EResourceType::CBV, CurrentStage, RootSignature, NumCBVs[CurrentStage]))
        {
            CommonState.ConstantBufferCache.DirtyResources(CurrentStage);
        }

        if (NumSRVs[CurrentStage] > 0 && CommonState.DescriptorCache.IsTableLayoutStale(EResourceType::SRV, CurrentStage, RootSignature, NumSRVs[CurrentStage]))
        {
            CommonState.ShaderResourceViewCache.DirtyResources(CurrentStage);
        }

        if (NumUAVs[CurrentStage] > 0 && CommonState.DescriptorCache.IsTableLayoutStale(EResourceType::UAV, CurrentStage, RootSignature, NumUAVs[CurrentStage]))
        {
            CommonState.UnorderedAccessViewCache.DirtyResources(CurrentStage);
        }
    }

    const uint32 StartHandleOffset = CommonState.DescriptorCache.GetResourceHeap().AllocateHandles(NumResourceDescriptors);
    uint32 DescriptorHandleOffset  = StartHandleOffset;

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        if (CommonState.ConstantBufferCache.IsResourcesDirty(CurrentStage) || GD3D12ForceBinding)
        {
            if (NumCBVs[CurrentStage] > 0)
            {
                CommonState.DescriptorCache.PrepareCBVs(CommonState.ConstantBufferCache, RootSignature, CurrentStage, NumCBVs[CurrentStage], DescriptorHandleOffset);
                CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
            }
            else
            {
                CommonState.ConstantBufferCache.ClearResourcesDirty(CurrentStage);
                CommonState.ConstantBufferCache.DirtyDescriptorTable(CurrentStage);
            }
        }
    }

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        if (NumSRVs[CurrentStage] > 0)
        {
            if (CommonState.ShaderResourceViewCache.IsResourcesDirty(CurrentStage) || GD3D12ForceBinding)
            {
                CommonState.DescriptorCache.PrepareSRVs(CommonState.ShaderResourceViewCache, RootSignature, CurrentStage, NumSRVs[CurrentStage], DescriptorHandleOffset);
                CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
            }
        }
    }

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        if (NumUAVs[CurrentStage] > 0)
        {
            if (CommonState.UnorderedAccessViewCache.IsResourcesDirty(CurrentStage) || GD3D12ForceBinding)
            {
                CommonState.DescriptorCache.PrepareUAVs(CommonState.UnorderedAccessViewCache, RootSignature, CurrentStage, NumUAVs[CurrentStage], DescriptorHandleOffset);
                CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
            }
        }
    }

    CommonState.DescriptorCache.GetResourceHeap().SetCurrentHandle(DescriptorHandleOffset);
    return bCommandListSplit;
}

void FD3D12CommandContextState::BindResources(FD3D12RootSignature* RootSignature, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage)
{
    CommonState.DescriptorCache.SetDescriptorHeaps();

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        const bool bDescriptorTableDirty = CommonState.ConstantBufferCache.IsDescriptorTableDirty(CurrentStage) || GD3D12ForceBinding;

        const uint32 NumCBVs = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::CBV);
        if (NumCBVs > 0 && bDescriptorTableDirty)
        {
            CommonState.DescriptorCache.BindCBVs(RootSignature, CurrentStage);
        }

        const FD3D12ShaderStage& Stage = RootSignature->GetShaderStage(CurrentStage);
        if (Stage.GetNumRootCBVs() > 0 && bDescriptorTableDirty)
        {
            auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[CurrentStage];
            for (uint8 RootCBVIdx = 0; RootCBVIdx < Stage.GetNumRootCBVs(); RootCBVIdx++)
            {
                const int8   ParamIndex = Stage.GetRootCBVParameterIndexBySlot(RootCBVIdx);
                const uint16 Register   = Stage.GetRootCBVRegister(RootCBVIdx);
                CHECK(Register < D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);

                D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;
                if (FD3D12BufferRHI* Buffer = CBVCache[Register])
                {
                    GPUVirtualAddress = Buffer->GetGPUVirtualAddress();
                    Context.GetCommandList().UpdateResidency(Buffer->GetResource()->GetResidencyHandle());
                }

                if (CurrentStage == EShaderVisibility::All)
                {
                    Context.GetCommandList()->SetComputeRootConstantBufferView(ParamIndex, GPUVirtualAddress);
                }
                else
                {
                    Context.GetCommandList()->SetGraphicsRootConstantBufferView(ParamIndex, GPUVirtualAddress);
                }
            }
        }

        if (bDescriptorTableDirty)
        {
            CommonState.ConstantBufferCache.ClearDescriptorTableDirty(CurrentStage);
        }
    }

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        const uint32 NumSRVs = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::SRV);
        if (NumSRVs > 0)
        {
            if (CommonState.ShaderResourceViewCache.IsDescriptorTableDirty(CurrentStage) || GD3D12ForceBinding)
            {
                CommonState.DescriptorCache.BindSRVs(RootSignature, CurrentStage);
                CommonState.ShaderResourceViewCache.ClearDescriptorTableDirty(CurrentStage);
            }
        }
    }

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        const uint32 NumUAVs = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::UAV);
        if (NumUAVs > 0)
        {
            if (CommonState.UnorderedAccessViewCache.IsDescriptorTableDirty(CurrentStage) || GD3D12ForceBinding)
            {
                CommonState.DescriptorCache.BindUAVs(RootSignature, CurrentStage);
                CommonState.UnorderedAccessViewCache.ClearDescriptorTableDirty(CurrentStage);
            }
        }
    }
}

void FD3D12CommandContextState::BindSamplers(FD3D12RootSignature* RootSignature, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage)
{
    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        const uint32 NumSamplers = RootSignature->GetMaxResourceCount(CurrentStage, EResourceType::Sampler);
        if (NumSamplers > 0)
        {
            if (CommonState.SamplerStateCache.IsDescriptorTableDirty(CurrentStage) || GD3D12ForceBinding)
            {
                CommonState.DescriptorCache.BindSamplers(RootSignature, CurrentStage);
                CommonState.SamplerStateCache.ClearDescriptorTableDirty(CurrentStage);
            }
        }
    }
}

void FD3D12CommandContextState::BindShaderConstants(FD3D12RootSignature* InRootSignature, EShaderConstantsPipeline::Type Pipeline)
{
    const int32 ParameterIndex = InRootSignature->Get32BitConstantsIndex();
    if (ParameterIndex < 0)
    {
        return;
    }

    const FD3D12ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache[Pipeline];

    const uint32 NumConstants = Math::Min(ConstantCache.NumConstants, InRootSignature->GetNum32BitConstants());
    if (NumConstants == 0)
    {
        return;
    }

    if (IsComputeRootSignatureSlot(Pipeline))
    {
        Context.GetCommandList()->SetComputeRoot32BitConstants(ParameterIndex, NumConstants, ConstantCache.Constants, 0);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRoot32BitConstants(ParameterIndex, NumConstants, ConstantCache.Constants, 0);
    }
}

void FD3D12CommandContextState::ResetState()
{
    CommonState.DescriptorCache.DirtyState();

    for (FD3D12ShaderConstantsCache& ConstantCache : CommonState.ShaderConstantsCache)
    {
        ConstantCache.Clear();
    }

    CommonState.ConstantBufferCache.Clear();
    CommonState.ShaderResourceViewCache.Clear();
    CommonState.UnorderedAccessViewCache.Clear();
    CommonState.SamplerStateCache.Clear();

    CommonGraphicsState.RenderTargetCache.Clear();
    GraphicsState.VertexBufferCache.Clear();
    GraphicsState.IndexBufferCache.Clear();

    Memory::Memzero(CommonGraphicsState.BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
    Memory::Memzero(CommonGraphicsState.Viewports, sizeof(CommonGraphicsState.Viewports));
    CommonGraphicsState.NumViewports = 0;

    Memory::Memzero(CommonGraphicsState.ScissorRects, sizeof(CommonGraphicsState.ScissorRects));
    CommonGraphicsState.NumScissorRects = 0;

    Memory::Memzero(CommonGraphicsState.SamplePositions, sizeof(CommonGraphicsState.SamplePositions));
    CommonGraphicsState.NumSamplesPerPixel      = 0;
    CommonGraphicsState.NumSamplePositionPixels = 0;
    CommonGraphicsState.bBindSamplePositions    = false;

    GraphicsState.PipelineState               = nullptr;
    CommonGraphicsState.ShadingRate           = D3D12_SHADING_RATE_1X1;
    CommonGraphicsState.ShadingRateImage      = nullptr;
    GraphicsState.bBindIndexBuffer            = true;
    CommonGraphicsState.bBindRenderTargets    = true;
    CommonGraphicsState.bBindBlendFactor      = true;
    GraphicsState.bBindPipelineState          = true;
    CommonGraphicsState.bBindScissorRects     = true;
    CommonGraphicsState.bBindViewports        = true;
    CommonGraphicsState.BoundRootSignature    = nullptr;
    ComputeCommonState.BoundRootSignature     = nullptr;
    CommonGraphicsState.bBindShadingRate      = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1;
    CommonGraphicsState.bBindShadingRateImage = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    GraphicsState.bBindVertexBuffers          = true;
    GraphicsState.bBindShaderConstants        = true;
    GraphicsState.bBindPrimitiveTopology      = true;

    ComputeState.PipelineState                = nullptr;
    ComputeState.bBindPipelineState           = true;
    ComputeState.bBindShaderConstants         = true;

    MeshletState.PipelineState                = nullptr;
    MeshletState.bBindPipelineState           = true;
    MeshletState.bBindShaderConstants         = true;

    RayTracingState.PipelineState             = nullptr;
    RayTracingState.bBindShaderConstants      = true;
}

void FD3D12CommandContextState::ResetStateResources()
{
    CommonState.DescriptorCache.DirtyDescriptorHeaps();
    CommonState.DescriptorCache.DirtyStateResources();

    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.UnorderedAccessViewCache.DirtyResourcesAll();
}

void FD3D12CommandContextState::BeginCommandList()
{
    CommonState.DescriptorCache.DirtyDescriptorHeaps();
    CommonState.DescriptorCache.DirtyStateResources();
    CommonState.DescriptorCache.InvalidateCachedSamplerTables();

    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.UnorderedAccessViewCache.DirtyResourcesAll();
    CommonState.SamplerStateCache.DirtyResourcesAll();

    GraphicsState.bBindIndexBuffer            = true;
    CommonGraphicsState.bBindRenderTargets    = true;
    CommonGraphicsState.bBindBlendFactor      = true;
    CommonGraphicsState.bBindStencilRef       = true;
    GraphicsState.bBindPipelineState          = true;
    CommonGraphicsState.bBindScissorRects     = true;
    CommonGraphicsState.bBindViewports        = true;
    CommonGraphicsState.BoundRootSignature    = nullptr;
    ComputeCommonState.BoundRootSignature     = nullptr;
    CommonGraphicsState.bBindShadingRate      = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1;
    CommonGraphicsState.bBindShadingRateImage = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    GraphicsState.bBindVertexBuffers          = true;
    GraphicsState.bBindShaderConstants        = true;
    GraphicsState.bBindPrimitiveTopology      = true;
    GraphicsState.bBindStreamOutputTargets    = (GraphicsState.NumSOBuffers > 0);

#if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
    CommonGraphicsState.bBindDepthBias        = true;
#endif

#if D3D12_ENABLE_DEPTH_BOUNDS_TEST && D3D12_USE_ID3D12COMMANDLIST_1
    CommonGraphicsState.bBindDepthBounds      = true;
#endif

    // A fresh command list already starts at the default positions, so only a custom pattern needs re-applying.
    CommonGraphicsState.bBindSamplePositions  = (CommonGraphicsState.NumSamplesPerPixel > 0);

    ComputeState.bBindPipelineState           = true;
    ComputeState.bBindShaderConstants         = true;

    MeshletState.bBindPipelineState           = true;
    MeshletState.bBindShaderConstants         = true;

    RayTracingState.bBindShaderConstants      = true;

    // Applied up front instead of being left dirty.
    FlushSamplePositions();
}

void FD3D12CommandContextState::SetGraphicsPipelineState(FD3D12GraphicsPipelineStateRHI* InGraphicsPipelineState)
{
    FD3D12GraphicsPipelineStateRHI* CurrentGraphicsPipelineState = GraphicsState.PipelineState.Get();
    if (CurrentGraphicsPipelineState != InGraphicsPipelineState)
    {
        FD3D12RootSignature* const RootSignature = InGraphicsPipelineState ? 
            InGraphicsPipelineState->GetRootSignature() :
            nullptr;

        FD3D12RootSignature* const CurrentRootSignature = CurrentGraphicsPipelineState ?
            CurrentGraphicsPipelineState->GetRootSignature() :
            nullptr;

        if (CurrentRootSignature != RootSignature)
        {
            DirtyAllResources();
        }

        const D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = InGraphicsPipelineState ? 
            InGraphicsPipelineState->GetD3D12PrimitiveTopology() : 
            D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

        const D3D12_PRIMITIVE_TOPOLOGY CurrentPrimitiveTopology = CurrentGraphicsPipelineState ? 
            CurrentGraphicsPipelineState->GetD3D12PrimitiveTopology() : 
            D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

        if (CurrentPrimitiveTopology != PrimitiveTopology)
        {
            GraphicsState.bBindPrimitiveTopology = true;
        }

        const bool bCurrentOverridesStencilRef = (CurrentGraphicsPipelineState != nullptr) && 
            IsEnumFlagSet(CurrentGraphicsPipelineState->GetShaderFlags(), ED3D12ShaderFlags::RequiresStencilRef);
        const bool bNewOverridesStencilRef = (InGraphicsPipelineState != nullptr) && 
            IsEnumFlagSet(InGraphicsPipelineState->GetShaderFlags(), ED3D12ShaderFlags::RequiresStencilRef);
        
        if (bCurrentOverridesStencilRef && !bNewOverridesStencilRef)
        {
            CommonGraphicsState.bBindStencilRef = true;
        }

        GraphicsState.PipelineState      = MakeSharedRef<FD3D12GraphicsPipelineStateRHI>(InGraphicsPipelineState);
        GraphicsState.bBindPipelineState = true;

    #if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
        if (GD3D12SupportDynamicDepthBias)
        {
            CommonGraphicsState.DepthBias[0]   = 0.0f;
            CommonGraphicsState.DepthBias[1]   = 0.0f;
            CommonGraphicsState.DepthBias[2]   = 0.0f;
            CommonGraphicsState.bBindDepthBias = true;
        }
    #endif

    #if D3D12_ENABLE_DEPTH_BOUNDS_TEST && D3D12_USE_ID3D12COMMANDLIST_1
        if (GD3D12DepthBoundsTestSupported && 
            (InGraphicsPipelineState == nullptr || !InGraphicsPipelineState->IsDepthBoundsTestEnabled()))
        {
            CommonGraphicsState.DepthBounds[0]   = 0.0f;
            CommonGraphicsState.DepthBounds[1]   = 1.0f;
            CommonGraphicsState.bBindDepthBounds = true;
        }
    #endif
    }
}

void FD3D12CommandContextState::SetComputePipelineState(FD3D12ComputePipelineStateRHI* InComputePipelineState)
{
    FD3D12ComputePipelineStateRHI* CurrentComputePipelineState = ComputeState.PipelineState.Get();
    if (CurrentComputePipelineState != InComputePipelineState)
    {
        FD3D12RootSignature* const RootSignature = InComputePipelineState ? 
            InComputePipelineState->GetRootSignature() : 
            nullptr;

        FD3D12RootSignature* const CurrentRootSignature = CurrentComputePipelineState ? 
            CurrentComputePipelineState->GetRootSignature() : 
            nullptr;

        if (CurrentRootSignature != RootSignature)
        {
            DirtyAllResources();
        }

        ComputeState.PipelineState      = MakeSharedRef<FD3D12ComputePipelineStateRHI>(InComputePipelineState);
        ComputeState.bBindPipelineState = true;
    }
}

void FD3D12CommandContextState::SetRayTracingPipelineState(FD3D12RayTracingPipelineStateRHI* InRayTracingPipelineState)
{
    FD3D12RayTracingPipelineStateRHI* CurrentRayTracingPipelineState = RayTracingState.PipelineState.Get();
    if (CurrentRayTracingPipelineState != InRayTracingPipelineState)
    {
        FD3D12RootSignature* const RootSignature = InRayTracingPipelineState ?
            InRayTracingPipelineState->GetGlobalRootSignature() : nullptr;

        FD3D12RootSignature* const CurrentRootSignature = CurrentRayTracingPipelineState ?
            CurrentRayTracingPipelineState->GetGlobalRootSignature() : nullptr;

        if (CurrentRootSignature != RootSignature)
        {
            DirtyAllResources();
        }

        RayTracingState.PipelineState = MakeSharedRef<FD3D12RayTracingPipelineStateRHI>(InRayTracingPipelineState);
    }
}

void FD3D12CommandContextState::SetMeshletPipelineState(FD3D12MeshletPipelineStateRHI* InMeshletPipelineState)
{
    FD3D12MeshletPipelineStateRHI* CurrentMeshletPipelineState = MeshletState.PipelineState.Get();
    if (CurrentMeshletPipelineState != InMeshletPipelineState)
    {
        FD3D12RootSignature* const RootSignature = InMeshletPipelineState ?
            InMeshletPipelineState->GetRootSignature() :
            nullptr;

        FD3D12RootSignature* const CurrentRootSignature = CurrentMeshletPipelineState ?
            CurrentMeshletPipelineState->GetRootSignature() :
            nullptr;

        if (CurrentRootSignature != RootSignature)
        {
            DirtyAllResources();
        }

        MeshletState.PipelineState      = MakeSharedRef<FD3D12MeshletPipelineStateRHI>(InMeshletPipelineState);
        MeshletState.bBindPipelineState = true;

    #if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
        if (GD3D12SupportDynamicDepthBias)
        {
            CommonGraphicsState.DepthBias[0]   = 0.0f;
            CommonGraphicsState.DepthBias[1]   = 0.0f;
            CommonGraphicsState.DepthBias[2]   = 0.0f;
            CommonGraphicsState.bBindDepthBias = true;
        }
    #endif

    #if D3D12_ENABLE_DEPTH_BOUNDS_TEST && D3D12_USE_ID3D12COMMANDLIST_1
        if (GD3D12DepthBoundsTestSupported && 
            (InMeshletPipelineState == nullptr || !InMeshletPipelineState->IsDepthBoundsTestEnabled()))
        {
            CommonGraphicsState.DepthBounds[0]   = 0.0f;
            CommonGraphicsState.DepthBounds[1]   = 1.0f;
            CommonGraphicsState.bBindDepthBounds = true;
        }
    #endif
    }
}

void FD3D12CommandContextState::SetRenderTargets(FD3D12RenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FD3D12DepthStencilViewRHI* DepthStencil)
{
    if (CommonGraphicsState.RenderTargetCache.DepthStencilView != DepthStencil)
    {
        CommonGraphicsState.RenderTargetCache.DepthStencilView = DepthStencil;
        CommonGraphicsState.bBindRenderTargets = true;
    }

    CHECK(NumRenderTargets < D3D12_MAX_RENDER_TARGET_COUNT);
    CommonGraphicsState.RenderTargetCache.NumRenderTargets = NumRenderTargets;

    for (uint32 Index = 0; Index < NumRenderTargets; Index++)
    {
        if (CommonGraphicsState.RenderTargetCache.RenderTargetViews[Index] != RenderTargets[Index])
        {
            CommonGraphicsState.RenderTargetCache.RenderTargetViews[Index] = RenderTargets[Index];
            CommonGraphicsState.bBindRenderTargets               = true;
        }
    }
}

void FD3D12CommandContextState::SetShadingRate(EShadingRate ShadingRate)
{
    D3D12_SHADING_RATE D3DShadingRate = ConvertShadingRate(ShadingRate);
    if (CommonGraphicsState.ShadingRate != D3DShadingRate)
    {
        CommonGraphicsState.ShadingRate      = D3DShadingRate;
        CommonGraphicsState.bBindShadingRate = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1;
    }
}

void FD3D12CommandContextState::SetShadingRateImage(FD3D12TextureRHI* ShadingRateImage)
{
    if (CommonGraphicsState.ShadingRateImage != ShadingRateImage)
    {
        CommonGraphicsState.ShadingRateImage      = ShadingRateImage;
        CommonGraphicsState.bBindShadingRateImage = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    }
}

void FD3D12CommandContextState::SetViewports(D3D12_VIEWPORT* Viewports, uint32 NumViewports)
{
    CHECK(NumViewports < D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ViewportArraySize = sizeof(D3D12_VIEWPORT) * NumViewports;
    if (CommonGraphicsState.NumViewports != NumViewports || Memory::Memcmp(CommonGraphicsState.Viewports, Viewports, ViewportArraySize) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.Viewports, Viewports, ViewportArraySize);

        CommonGraphicsState.NumViewports   = NumViewports;
        CommonGraphicsState.bBindViewports = true;
    }
}

void FD3D12CommandContextState::SetScissorRects(D3D12_RECT* ScissorRects, uint32 NumScissorRects)
{
    CHECK(NumScissorRects < D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ScissorRectArraySize = sizeof(D3D12_RECT) * NumScissorRects;
    if (CommonGraphicsState.NumScissorRects != NumScissorRects || Memory::Memcmp(CommonGraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize);

        CommonGraphicsState.NumScissorRects   = NumScissorRects;
        CommonGraphicsState.bBindScissorRects = true;
    }
}

void FD3D12CommandContextState::SetBlendFactor(const float BlendFactor[4])
{
    if (Memory::Memcmp(CommonGraphicsState.BlendFactor, BlendFactor, sizeof(CommonGraphicsState.BlendFactor)) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.BlendFactor, BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
        CommonGraphicsState.bBindBlendFactor = true;
    }
}

void FD3D12CommandContextState::SetStencilRef(uint32 InStencilRef)
{
    if (CommonGraphicsState.StencilRef != InStencilRef)
    {
        CommonGraphicsState.StencilRef      = InStencilRef;
        CommonGraphicsState.bBindStencilRef = true;

        FD3D12GraphicsPipelineStateRHI* BoundPSO = GraphicsState.PipelineState.Get();
        if (BoundPSO != nullptr && IsEnumFlagSet(BoundPSO->GetShaderFlags(), ED3D12ShaderFlags::RequiresStencilRef))
        {
            String PSOName;
            BoundPSO->GetDebugName(PSOName);

            if (PSOName.IsEmpty())
            {
                PSOName = "<unnamed>";
            }

            D3D12_WARNING("[FD3D12CommandContextState] SetStencilRef(%u) called but bound PSO '%s' overrides the OM stencil ref per-pixel via SV_StencilRef. This value will be ignored.", InStencilRef, *PSOName);
        }
    }
}

void FD3D12CommandContextState::SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias)
{
    const float NewValues[3] = 
    { 
        InDepthBias, 
        InDepthBiasClamp, 
        InSlopeScaledDepthBias 
    };
    
    if (Memory::Memcmp(CommonGraphicsState.DepthBias, NewValues, sizeof(NewValues)) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.DepthBias, NewValues, sizeof(NewValues));
        CommonGraphicsState.bBindDepthBias = true;
    }
}

void FD3D12CommandContextState::SetDepthBounds(float InMinDepth, float InMaxDepth)
{
    const float NewValues[2] = 
    { 
        InMinDepth, 
        InMaxDepth 
    };

    if (Memory::Memcmp(CommonGraphicsState.DepthBounds, NewValues, sizeof(NewValues)) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.DepthBounds, NewValues, sizeof(NewValues));
        CommonGraphicsState.bBindDepthBounds = true;
    }
}

void FD3D12CommandContextState::SetSamplePositions(const D3D12_SAMPLE_POSITION* InSamplePositions, uint32 InNumSamplesPerPixel, uint32 InNumPixels)
{
    const uint32 NumPositions = InNumSamplesPerPixel * InNumPixels;
    CHECK(NumPositions <= RHI_MAX_SAMPLE_POSITIONS);

    const uint32 PositionArraySize = sizeof(D3D12_SAMPLE_POSITION) * NumPositions;
    if (CommonGraphicsState.NumSamplesPerPixel      != InNumSamplesPerPixel ||
        CommonGraphicsState.NumSamplePositionPixels != InNumPixels          ||
        Memory::Memcmp(CommonGraphicsState.SamplePositions, InSamplePositions, PositionArraySize) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.SamplePositions, InSamplePositions, PositionArraySize);

        CommonGraphicsState.NumSamplesPerPixel       = InNumSamplesPerPixel;
        CommonGraphicsState.NumSamplePositionPixels  = InNumPixels;
        CommonGraphicsState.bBindSamplePositions     = true;
    }

    // D3D12 associates sample positions with depth-buffer contents, so they have to be live for the
    // clears and transitions that follow rather than only for the next draw.
    FlushSamplePositions();
}

void FD3D12CommandContextState::FlushDepthBias()
{
#if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
    if (CommonGraphicsState.bBindDepthBias && Context.GetCommandList().GetGraphicsCommandList9().IsValid())
    {
        Context.GetCommandList().GetGraphicsCommandList9()->RSSetDepthBias(
            CommonGraphicsState.DepthBias[0],
            CommonGraphicsState.DepthBias[1],
            CommonGraphicsState.DepthBias[2]);

        CommonGraphicsState.bBindDepthBias = false;
    }
#endif
}

void FD3D12CommandContextState::FlushDepthBounds()
{
#if D3D12_ENABLE_DEPTH_BOUNDS_TEST && D3D12_USE_ID3D12COMMANDLIST_1
    if (CommonGraphicsState.bBindDepthBounds && Context.GetCommandList().GetGraphicsCommandList1().IsValid())
    {
        Context.GetCommandList().GetGraphicsCommandList1()->OMSetDepthBounds(
            CommonGraphicsState.DepthBounds[0],
            CommonGraphicsState.DepthBounds[1]);

        CommonGraphicsState.bBindDepthBounds = false;
    }
#endif
}

void FD3D12CommandContextState::FlushSamplePositions()
{
#if D3D12_USE_ID3D12COMMANDLIST_1
    if (CommonGraphicsState.bBindSamplePositions && Context.GetCommandList().GetGraphicsCommandList1().IsValid())
    {
        // Zero counts restore the hardware defaults.
        Context.GetCommandList().GetGraphicsCommandList1()->SetSamplePositions(
            CommonGraphicsState.NumSamplesPerPixel,
            CommonGraphicsState.NumSamplePositionPixels,
            CommonGraphicsState.NumSamplesPerPixel > 0 ? CommonGraphicsState.SamplePositions : nullptr);

        CommonGraphicsState.bBindSamplePositions = false;
    }
#endif
}

void FD3D12CommandContextState::FlushDefaultSamplePositions()
{
#if D3D12_USE_ID3D12COMMANDLIST_1
    if (CommonGraphicsState.NumSamplesPerPixel > 0 && Context.GetCommandList().GetGraphicsCommandList1().IsValid())
    {
        Context.GetCommandList().GetGraphicsCommandList1()->SetSamplePositions(0, 0, nullptr);

        // Keep the cached pattern dirty so the paired FlushSamplePositions can put it back.
        CommonGraphicsState.bBindSamplePositions = true;
    }
#endif
}

void FD3D12CommandContextState::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    GraphicsState.NumSOBuffers = Math::Min(static_cast<uint32>(Buffers.Size()), 4u);

    for (uint32 Index = 0; Index < GraphicsState.NumSOBuffers; ++Index)
    {
        FD3D12BufferRHI* D3DBuffer = FD3D12DeviceRHI::ResourceCast(Buffers[Index]);
        GraphicsState.SOBuffers[Index] = D3DBuffer;

        if (D3DBuffer)
        {
            GraphicsState.SOBufferViews[Index].BufferLocation           = D3DBuffer->GetGPUVirtualAddress() + (Offsets ? Offsets[Index] : 0);
            GraphicsState.SOBufferViews[Index].SizeInBytes              = D3DBuffer->GetDesc().Size;
            GraphicsState.SOBufferViews[Index].BufferFilledSizeLocation = 0;
        }
        else
        {
            Memory::Memzero(&GraphicsState.SOBufferViews[Index], sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW));
        }
    }

    GraphicsState.bBindStreamOutputTargets = true;
}

void FD3D12CommandContextState::SetVertexBuffer(FD3D12BufferRHI* VertexBuffer, uint32 VertexBufferSlot)
{
    CHECK(VertexBufferSlot < D3D12_MAX_VERTEX_BUFFER_SLOTS);

    D3D12_VERTEX_BUFFER_VIEW CurrentVBV;
    if (VertexBuffer)
    {
        CurrentVBV.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
        CurrentVBV.SizeInBytes    = static_cast<uint32>(VertexBuffer->GetDesc().Size);
        CurrentVBV.StrideInBytes  = VertexBuffer->GetDesc().Stride;
    }
    else
    {
        Memory::Memzero(&CurrentVBV);
    }

    if (Memory::Memcmp(&CurrentVBV, &GraphicsState.VertexBufferCache.VertexBuffers[VertexBufferSlot], sizeof(D3D12_VERTEX_BUFFER_VIEW)) != 0)
    {
        Memory::Memcpy(&GraphicsState.VertexBufferCache.VertexBuffers[VertexBufferSlot], &CurrentVBV, sizeof(D3D12_VERTEX_BUFFER_VIEW));
        GraphicsState.VertexBufferCache.BufferResources[VertexBufferSlot] = VertexBuffer;

        const uint8 NumVertexBuffers = uint8(Math::Max<uint32>(GraphicsState.VertexBufferCache.NumVertexBuffers, VertexBufferSlot + 1));
        GraphicsState.VertexBufferCache.NumVertexBuffers = NumVertexBuffers;
        GraphicsState.bBindVertexBuffers                 = true;
    }
}

void FD3D12CommandContextState::SetIndexBuffer(FD3D12BufferRHI* IndexBuffer, DXGI_FORMAT IndexFormat)
{
    D3D12_INDEX_BUFFER_VIEW NewIndexBuffer;
    if (IndexBuffer)
    {
        NewIndexBuffer.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
        NewIndexBuffer.Format         = IndexFormat;
        NewIndexBuffer.SizeInBytes    = static_cast<uint32>(IndexBuffer->GetDesc().Size);
    }
    else
    {
        Memory::Memzero(&NewIndexBuffer);
    }

    if (Memory::Memcmp(&NewIndexBuffer, &GraphicsState.IndexBufferCache.IndexBuffer, sizeof(D3D12_INDEX_BUFFER_VIEW)) != 0)
    {
        Memory::Memcpy(&GraphicsState.IndexBufferCache.IndexBuffer, &NewIndexBuffer, sizeof(D3D12_INDEX_BUFFER_VIEW));
        GraphicsState.IndexBufferCache.BufferResource = IndexBuffer;
        GraphicsState.bBindIndexBuffer = true;
    }
}

void FD3D12CommandContextState::SetSRV(FD3D12ShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    auto& SRVCache = CommonState.ShaderResourceViewCache.ResourceViews[ShaderStage];
    if (SRVCache[ResourceIndex] != ShaderResourceView)
    {
        SRVCache[ResourceIndex] = ShaderResourceView;

        const uint8 NumViews = Math::Max<uint8>(CommonState.ShaderResourceViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ShaderResourceViewCache.ViewVersions[ShaderStage][ResourceIndex] = ShaderResourceView ? ShaderResourceView->GetDescriptorVersion() : 0;
        CommonState.ShaderResourceViewCache.NumViews[ShaderStage]                    = NumViews;
        CommonState.ShaderResourceViewCache.DirtyResources(ShaderStage);
    }
}

void FD3D12CommandContextState::SetUAV(FD3D12UnorderedAccessViewRHI* UnorderedAccessView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    auto& UAVCache = CommonState.UnorderedAccessViewCache.ResourceViews[ShaderStage];
    if (UAVCache[ResourceIndex] != UnorderedAccessView)
    {
        UAVCache[ResourceIndex] = UnorderedAccessView;

        const uint8 NumViews = Math::Max<uint8>(CommonState.UnorderedAccessViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.UnorderedAccessViewCache.ViewVersions[ShaderStage][ResourceIndex] = UnorderedAccessView ? UnorderedAccessView->GetDescriptorVersion() : 0;
        CommonState.UnorderedAccessViewCache.NumViews[ShaderStage]                    = NumViews;

        CommonState.UnorderedAccessViewCache.DirtyResources(ShaderStage);
    }
}

void FD3D12CommandContextState::SetCBV(FD3D12BufferRHI* Buffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[ShaderStage];
    if (CBVCache[ResourceIndex] != Buffer)
    {
        CBVCache[ResourceIndex] = Buffer;

        const uint8 NumBuffers = Math::Max<uint8>(CommonState.ConstantBufferCache.NumBuffers[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ConstantBufferCache.ViewVersions[ShaderStage][ResourceIndex] = 0;
        CommonState.ConstantBufferCache.NumBuffers[ShaderStage]                  = NumBuffers;

        CommonState.ConstantBufferCache.DirtyResources(ShaderStage);
    }
}

void FD3D12CommandContextState::SetSampler(FD3D12SamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex)
{
    auto& SamplerCache = CommonState.SamplerStateCache.SamplerStates[ShaderStage];
    if (SamplerCache[SamplerIndex] != SamplerState)
    {
        SamplerCache[SamplerIndex] = SamplerState;

        const uint8 NumSamplers = Math::Max<uint8>(CommonState.SamplerStateCache.NumSamplers[ShaderStage], static_cast<uint8>(SamplerIndex) + 1);
        CommonState.SamplerStateCache.NumSamplers[ShaderStage] = NumSamplers;
        
        CommonState.SamplerStateCache.DirtyResources(ShaderStage);
    }
}

void FD3D12CommandContextState::SetShaderConstants(EShaderStage ShaderStage, const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    const EShaderConstantsPipeline::Type Pipeline = GetShaderConstantsPipeline(ShaderStage);

    FD3D12ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache[Pipeline];
    if (NumShaderConstants != ConstantCache.NumConstants || Memory::Memcmp(ShaderConstants, ConstantCache.Constants, sizeof(uint32) * NumShaderConstants) != 0)
    {
        Memory::Memcpy(ConstantCache.Constants, ShaderConstants, sizeof(uint32) * NumShaderConstants);
        ConstantCache.NumConstants = NumShaderConstants;

        DirtyShaderConstants(Pipeline);
    }
}

void FD3D12CommandContextState::DirtyShaderConstants(EShaderConstantsPipeline::Type Pipeline)
{
    switch (Pipeline)
    {
    case EShaderConstantsPipeline::Graphics:
        GraphicsState.bBindShaderConstants = true;
        MeshletState.bBindShaderConstants  = true;
        break;
    case EShaderConstantsPipeline::Compute:
        ComputeState.bBindShaderConstants = true;
        break;
    case EShaderConstantsPipeline::RayTracing:
        RayTracingState.bBindShaderConstants = true;
        break;
    default:
        break;
    }
}

void FD3D12CommandContextState::DirtyAllResources()
{
    CommonState.ConstantBufferCache.DirtyResources(EShaderVisibility::All);
    CommonState.ShaderResourceViewCache.DirtyResources(EShaderVisibility::All);
    CommonState.UnorderedAccessViewCache.DirtyResources(EShaderVisibility::All);
    CommonState.SamplerStateCache.DirtyResources(EShaderVisibility::All);
}

void FD3D12CommandContextState::InternalSetRootSignature(FD3D12RootSignature* InRootSignature, bool bIsCompute)
{
    FD3D12RootSignature* CurrentRootSignature = bIsCompute ? ComputeCommonState.BoundRootSignature : CommonGraphicsState.BoundRootSignature;
    if (CurrentRootSignature == InRootSignature)
    {
        return;
    }

    if (bIsCompute)
    {
        Context.GetCommandList()->SetComputeRootSignature(InRootSignature->GetD3D12RootSignature());
        ComputeCommonState.BoundRootSignature = InRootSignature;
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootSignature(InRootSignature->GetD3D12RootSignature());
        CommonGraphicsState.BoundRootSignature = InRootSignature;
    }

#if D3D12_ENABLE_PIPELINE_BIND_LOGGING
    D3D12_INFO("[PSO-BIND] Switched %s root signature", bIsCompute ? "compute" : "graphics");
#endif

    CommonState.ConstantBufferCache.DirtyDescriptorTableAll();
    CommonState.ShaderResourceViewCache.DirtyDescriptorTableAll();
    CommonState.UnorderedAccessViewCache.DirtyDescriptorTableAll();
    CommonState.SamplerStateCache.DirtyDescriptorTableAll();

    if (bIsCompute)
    {
        DirtyShaderConstants(EShaderConstantsPipeline::Compute);
        DirtyShaderConstants(EShaderConstantsPipeline::RayTracing);
    }
    else
    {
        DirtyShaderConstants(EShaderConstantsPipeline::Graphics);
    }
}
