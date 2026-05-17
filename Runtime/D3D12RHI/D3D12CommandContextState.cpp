#include "Core/Memory/Memory.h"
#include "D3D12RHI/D3D12CommandContextState.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12RHI.h"

FD3D12CommandContextState::FD3D12CommandContextState(FD3D12Device* InDevice, FD3D12CommandContext& InContext)
    : FD3D12DeviceChild(InDevice)
    , Context(InContext)
    , GraphicsState()
    , ComputeState()
    , CommonState(InDevice, InContext)
{
}

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
    FD3D12RootSignature* RootSignature = GraphicsState.PipelineState->GetRootSignature();

    bool bCommandListSplit;
    do
    {
        bCommandListSplit = false;

        bCommandListSplit |= PrepareResources(RootSignature, GraphicsState.PipelineState.Get(), EShaderVisibility::Vertex, EShaderVisibility::Pixel);
        bCommandListSplit |= PrepareSamplers(RootSignature, GraphicsState.PipelineState.Get(), EShaderVisibility::Vertex, EShaderVisibility::Pixel);
    } while (bCommandListSplit);

    FD3D12RenderTargetCache& RenderTargetCache = GraphicsState.RenderTargetCache;
    for (uint32 i = 0; i < RenderTargetCache.NumRenderTargets; i++)
    {
        if (FD3D12RenderTargetViewRHI* RenderTargetView = RenderTargetCache.RenderTargetViews[i])
        {
            Context.TransitionResourceState(RenderTargetView);
        }
    }

    if (FD3D12DepthStencilViewRHI* DepthStencilView = RenderTargetCache.DepthStencilView)
    {
        const D3D12_RESOURCE_STATES DesiredState = DepthStencilView->IsReadOnly()
            ? D3D12_RESOURCE_STATE_DEPTH_READ
            : D3D12_RESOURCE_STATE_DEPTH_WRITE;

        Context.TransitionResourceState(DepthStencilView, DesiredState);
    }

    for (uint32 i = 0; i < GraphicsState.VertexBufferCache.NumVertexBuffers; i++)
    {
        if (FD3D12BufferRHI* Buffer = GraphicsState.VertexBufferCache.BufferResources[i])
        {
            if (Buffer->GetResource()->RequiresResourceStateTracking())
            {
                Context.TransitionResourceState(Buffer->GetResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
        }
    }

    if (FD3D12BufferRHI* Buffer = GraphicsState.IndexBufferCache.BufferResource)
    {
        if (Buffer->GetResource()->RequiresResourceStateTracking())
        {
            Context.TransitionResourceState(Buffer->GetResource(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
        }
    }

    for (uint32 i = 0; i < GraphicsState.NumSOBuffers; i++)
    {
        if (FD3D12BufferRHI* Buffer = GraphicsState.SOBuffers[i])
        {
            if (Buffer->GetResource()->RequiresResourceStateTracking())
            {
                Context.TransitionResourceState(Buffer->GetResource(), D3D12_RESOURCE_STATE_STREAM_OUT);
            }
        }
    }

#if D3D12_USE_ID3D12COMMANDLIST_5
    if (FD3D12TextureRHI* ShadingRateTexture = GraphicsState.ShadingRateImage)
    {
        if (ShadingRateTexture->GetResource()->RequiresResourceStateTracking())
        {
            Context.TransitionResourceState(ShadingRateTexture->GetResource(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
        }
    }
#endif
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

    if (GraphicsState.bBindRootSignature)
    {
        InternalSetRootSignature(RootSignature, EShaderVisibility::Pixel);
    }

    if (GraphicsState.bBindRenderTargets)
    {
        CommonState.DescriptorCache.SetRenderTargets(GraphicsState.RenderTargetCache);
        GraphicsState.bBindRenderTargets = false;
    }

#if D3D12_USE_ID3D12COMMANDLIST_5
    if (Context.GetCommandList().GetGraphicsCommandList5().IsValid())
    {
        if (GraphicsState.bBindShadingRateImage)
        {
            ID3D12Resource* Resource = GraphicsState.ShadingRateImage ? GraphicsState.ShadingRateImage->GetResource()->GetD3D12Resource() : nullptr;
            Context.GetCommandList().GetGraphicsCommandList5()->RSSetShadingRateImage(Resource);
            GraphicsState.bBindShadingRateImage = false;

            if (GraphicsState.ShadingRateImage)
            {
                Context.GetCommandList().UpdateResidency(GraphicsState.ShadingRateImage->GetResource()->GetResidencyHandle());
            }
        }

        if (GraphicsState.bBindShadingRate)
        {
            D3D12_SHADING_RATE_COMBINER Combiners[] =
            {
                D3D12_SHADING_RATE_COMBINER_OVERRIDE,
                D3D12_SHADING_RATE_COMBINER_OVERRIDE,
            };

            Context.GetCommandList().GetGraphicsCommandList5()->RSSetShadingRate(GraphicsState.ShadingRate, Combiners);
            GraphicsState.bBindShadingRate = false;
        }
    }
#endif

    BindResources(RootSignature, EShaderVisibility::Vertex, EShaderVisibility::Pixel);
    BindSamplers(RootSignature, EShaderVisibility::Vertex, EShaderVisibility::Pixel);

    if (GraphicsState.bBindShaderConstants)
    {
        BindShaderConstants(RootSignature, EShaderVisibility::Pixel);
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

    if (GraphicsState.bBindViewports)
    {
        Context.GetCommandList()->RSSetViewports(GraphicsState.NumViewports, GraphicsState.Viewports);
        GraphicsState.bBindViewports = false;
    }

    if (GraphicsState.bBindScissorRects)
    {
        Context.GetCommandList()->RSSetScissorRects(GraphicsState.NumScissorRects, GraphicsState.ScissorRects);
        GraphicsState.bBindScissorRects = false;
    }

    if (GraphicsState.bBindBlendFactor)
    {
        Context.GetCommandList()->OMSetBlendFactor(GraphicsState.BlendFactor);
        GraphicsState.bBindBlendFactor = false;
    }

    if (GraphicsState.bBindStencilRef)
    {
        Context.GetCommandList()->OMSetStencilRef(GraphicsState.StencilRef);
        GraphicsState.bBindStencilRef = false;
    }

#if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
    if (GraphicsState.bBindDepthBias)
    {
        Context.GetCommandList().GetGraphicsCommandList9()->RSSetDepthBias(GraphicsState.DepthBias[0], GraphicsState.DepthBias[1], GraphicsState.DepthBias[2]);
        GraphicsState.bBindDepthBias = false;
    }
#endif

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
    FD3D12RootSignature* RootSignature = ComputeState.PipelineState->GetRootSignature();

    bool bCommandListSplit;
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

    if (ComputeState.bBindRootSignature)
    {
        InternalSetRootSignature(RootSignature, EShaderVisibility::All);
    }

    BindResources(RootSignature, EShaderVisibility::All, EShaderVisibility::All);
    BindSamplers(RootSignature, EShaderVisibility::All, EShaderVisibility::All);

    if (ComputeState.bBindShaderConstants)
    {
        BindShaderConstants(RootSignature, EShaderVisibility::All);
        ComputeState.bBindShaderConstants = false;
    }
}

bool FD3D12CommandContextState::PrepareSamplers(FD3D12RootSignature* RootSignature, FD3D12PipelineState* PipelineState, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage)
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
            if (GD3D12ResourceBindingTier > D3D12_RESOURCE_BINDING_TIER_1)
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

bool FD3D12CommandContextState::PrepareResources(FD3D12RootSignature* RootSignature, FD3D12PipelineState* PipelineState, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage)
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
            if (GD3D12ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3)
            {
                NumCBVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::CBV);
                NumSRVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::SRV);
                NumUAVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, EResourceType::UAV);
            }
            else if (GD3D12ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2)
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
            ResetStateResources();
            continue;
        }

        break;
    }

    for (EShaderVisibility::Type CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility::Type(CurrentStage + 1))
    {
        const D3D12_RESOURCE_STATES SRVState = (CurrentStage == EShaderVisibility::Pixel)
            ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

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
                    if (Buffer->GetResource()->RequiresResourceStateTracking())
                    {
                        Context.TransitionResourceState(Buffer->GetResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    }

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
                    if (Buffer->GetResource()->RequiresResourceStateTracking())
                    {
                        Context.TransitionResourceState(Buffer->GetResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    }

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
                    Context.TransitionResourceState(View, SRVState);

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

void FD3D12CommandContextState::BindShaderConstants(FD3D12RootSignature* InRootSignature, EShaderVisibility::Type ShaderStage)
{
    int32 ParameterIndex = InRootSignature->Get32BitConstantsIndex();
    if (ParameterIndex >= 0)
    {
        FD3D12ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache;
        if (ShaderStage == EShaderVisibility::All)
        {
            Context.GetCommandList()->SetComputeRoot32BitConstants(ParameterIndex, ConstantCache.NumConstants, ConstantCache.Constants, 0);
        }
        else
        {
            Context.GetCommandList()->SetGraphicsRoot32BitConstants(ParameterIndex, ConstantCache.NumConstants, ConstantCache.Constants, 0);
        }
    }
}

void FD3D12CommandContextState::ResetState()
{
    CommonState.DescriptorCache.DirtyState();
    CommonState.ShaderConstantsCache.Clear();

    CommonState.ConstantBufferCache.Clear();
    CommonState.ShaderResourceViewCache.Clear();
    CommonState.UnorderedAccessViewCache.Clear();
    CommonState.SamplerStateCache.Clear();

    GraphicsState.RenderTargetCache.Clear();
    GraphicsState.VertexBufferCache.Clear();
    GraphicsState.IndexBufferCache.Clear();

    FMemory::Memzero(GraphicsState.BlendFactor, sizeof(GraphicsState.BlendFactor));
    FMemory::Memzero(GraphicsState.Viewports, sizeof(GraphicsState.Viewports));
    GraphicsState.NumViewports = 0;

    FMemory::Memzero(GraphicsState.ScissorRects, sizeof(GraphicsState.ScissorRects));
    GraphicsState.NumScissorRects = 0;
    
    GraphicsState.PipelineState          = nullptr;
    GraphicsState.ShadingRate            = D3D12_SHADING_RATE_1X1;
    GraphicsState.ShadingRateImage       = nullptr;
    GraphicsState.bBindIndexBuffer       = true;
    GraphicsState.bBindRenderTargets     = true;
    GraphicsState.bBindBlendFactor       = true;
    GraphicsState.bBindPipelineState     = true;
    GraphicsState.bBindScissorRects      = true;
    GraphicsState.bBindViewports         = true;
    GraphicsState.bBindRootSignature     = true;
    GraphicsState.bBindShadingRate       = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1;
    GraphicsState.bBindShadingRateImage  = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    GraphicsState.bBindVertexBuffers     = true;
    GraphicsState.bBindShaderConstants   = true;
    GraphicsState.bBindPrimitiveTopology = true;

    ComputeState.PipelineState           = nullptr;
    ComputeState.bBindPipelineState      = true;
    ComputeState.bBindRootSignature      = true;
    ComputeState.bBindShaderConstants    = true;
}

void FD3D12CommandContextState::ResetStateResources()
{
    CommonState.DescriptorCache.DirtyDescriptorHeaps();
    CommonState.DescriptorCache.DirtyStateResources();

    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.UnorderedAccessViewCache.DirtyResourcesAll();
}

void FD3D12CommandContextState::ResetStateForNewCommandList()
{
    CommonState.DescriptorCache.DirtyDescriptorHeaps();
    CommonState.DescriptorCache.DirtyStateResources();
    CommonState.DescriptorCache.InvalidateCachedSamplerTables();

    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.UnorderedAccessViewCache.DirtyResourcesAll();
    CommonState.SamplerStateCache.DirtyResourcesAll();

    GraphicsState.bBindIndexBuffer          = true;
    GraphicsState.bBindRenderTargets        = true;
    GraphicsState.bBindBlendFactor          = true;
    GraphicsState.bBindStencilRef           = true;
    GraphicsState.bBindPipelineState        = true;
    GraphicsState.bBindScissorRects         = true;
    GraphicsState.bBindViewports            = true;
    GraphicsState.bBindRootSignature        = true;
    GraphicsState.bBindShadingRate          = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1;
    GraphicsState.bBindShadingRateImage     = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    GraphicsState.bBindVertexBuffers        = true;
    GraphicsState.bBindShaderConstants      = true;
    GraphicsState.bBindPrimitiveTopology    = true;
    GraphicsState.bBindStreamOutputTargets  = (GraphicsState.NumSOBuffers > 0);

#if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
    GraphicsState.bBindDepthBias            = true;
#endif

    ComputeState.bBindPipelineState         = true;
    ComputeState.bBindRootSignature         = true;
    ComputeState.bBindShaderConstants       = true;
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
            GraphicsState.bBindRootSignature = true;
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

        GraphicsState.PipelineState      = MakeSharedRef<FD3D12GraphicsPipelineStateRHI>(InGraphicsPipelineState);
        GraphicsState.bBindPipelineState = true;

    #if D3D12_ENABLE_DYNAMIC_DEPTH_BIAS && D3D12_USE_ID3D12COMMANDLIST_9
        if (GD3D12SupportDynamicDepthBias)
        {
            GraphicsState.DepthBias[0]   = 0.0f;
            GraphicsState.DepthBias[1]   = 0.0f;
            GraphicsState.DepthBias[2]   = 0.0f;
            GraphicsState.bBindDepthBias = true;
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
            ComputeState.bBindRootSignature = true;
        }

        ComputeState.PipelineState      = MakeSharedRef<FD3D12ComputePipelineStateRHI>(InComputePipelineState);
        ComputeState.bBindPipelineState = true;
    }
}

void FD3D12CommandContextState::SetRenderTargets(FD3D12RenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FD3D12DepthStencilViewRHI* DepthStencil)
{
    if (GraphicsState.RenderTargetCache.DepthStencilView != DepthStencil)
    {
        GraphicsState.RenderTargetCache.DepthStencilView = DepthStencil;
        GraphicsState.bBindRenderTargets = true;
    }

    CHECK(NumRenderTargets < D3D12_MAX_RENDER_TARGET_COUNT);
    GraphicsState.RenderTargetCache.NumRenderTargets = NumRenderTargets;

    for (uint32 Index = 0; Index < NumRenderTargets; Index++)
    {
        if (GraphicsState.RenderTargetCache.RenderTargetViews[Index] != RenderTargets[Index])
        {
            GraphicsState.RenderTargetCache.RenderTargetViews[Index] = RenderTargets[Index];
            GraphicsState.bBindRenderTargets               = true;
        }
    }
}

void FD3D12CommandContextState::SetShadingRate(EShadingRate ShadingRate)
{
    D3D12_SHADING_RATE D3DShadingRate = ConvertShadingRate(ShadingRate);
    if (GraphicsState.ShadingRate != D3DShadingRate)
    {
        GraphicsState.ShadingRate      = D3DShadingRate;
        GraphicsState.bBindShadingRate = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1;
    }
}

void FD3D12CommandContextState::SetShadingRateImage(FD3D12TextureRHI* ShadingRateImage)
{
    if (GraphicsState.ShadingRateImage != ShadingRateImage)
    {
        GraphicsState.ShadingRateImage      = ShadingRateImage;
        GraphicsState.bBindShadingRateImage = GD3D12VariableRateShadingTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    }
}

void FD3D12CommandContextState::SetViewports(D3D12_VIEWPORT* Viewports, uint32 NumViewports)
{
    CHECK(NumViewports < D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ViewportArraySize = sizeof(D3D12_VIEWPORT) * NumViewports;
    if (GraphicsState.NumViewports != NumViewports || FMemory::Memcmp(GraphicsState.Viewports, Viewports, ViewportArraySize) != 0)
    {
        FMemory::Memcpy(GraphicsState.Viewports, Viewports, ViewportArraySize);

        GraphicsState.NumViewports   = NumViewports;
        GraphicsState.bBindViewports = true;
    }
}

void FD3D12CommandContextState::SetScissorRects(D3D12_RECT* ScissorRects, uint32 NumScissorRects)
{
    CHECK(NumScissorRects < D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ScissorRectArraySize = sizeof(D3D12_RECT) * NumScissorRects;
    if (GraphicsState.NumScissorRects != NumScissorRects || FMemory::Memcmp(GraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize) != 0)
    {
        FMemory::Memcpy(GraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize);

        GraphicsState.NumScissorRects   = NumScissorRects;
        GraphicsState.bBindScissorRects = true;
    }
}

void FD3D12CommandContextState::SetBlendFactor(const float BlendFactor[4])
{
    if (FMemory::Memcmp(GraphicsState.BlendFactor, BlendFactor, sizeof(GraphicsState.BlendFactor)) != 0)
    {
        FMemory::Memcpy(GraphicsState.BlendFactor, BlendFactor, sizeof(GraphicsState.BlendFactor));
        GraphicsState.bBindBlendFactor = true;
    }
}

void FD3D12CommandContextState::SetStencilRef(uint32 InStencilRef)
{
    if (GraphicsState.StencilRef != InStencilRef)
    {
        GraphicsState.StencilRef      = InStencilRef;
        GraphicsState.bBindStencilRef = true;
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
    
    if (FMemory::Memcmp(GraphicsState.DepthBias, NewValues, sizeof(NewValues)) != 0)
    {
        FMemory::Memcpy(GraphicsState.DepthBias, NewValues, sizeof(NewValues));
        GraphicsState.bBindDepthBias = true;
    }
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
            FMemory::Memzero(&GraphicsState.SOBufferViews[Index], sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW));
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
        FMemory::Memzero(&CurrentVBV);
    }

    if (FMemory::Memcmp(&CurrentVBV, &GraphicsState.VertexBufferCache.VertexBuffers[VertexBufferSlot], sizeof(D3D12_VERTEX_BUFFER_VIEW)) != 0)
    {
        FMemory::Memcpy(&GraphicsState.VertexBufferCache.VertexBuffers[VertexBufferSlot], &CurrentVBV, sizeof(D3D12_VERTEX_BUFFER_VIEW));
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
        FMemory::Memzero(&NewIndexBuffer);
    }

    if (FMemory::Memcmp(&NewIndexBuffer, &GraphicsState.IndexBufferCache.IndexBuffer, sizeof(D3D12_INDEX_BUFFER_VIEW)) != 0)
    {
        FMemory::Memcpy(&GraphicsState.IndexBufferCache.IndexBuffer, &NewIndexBuffer, sizeof(D3D12_INDEX_BUFFER_VIEW));
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

void FD3D12CommandContextState::SetShaderConstants(const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    FD3D12ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache;
    if (NumShaderConstants != ConstantCache.NumConstants || FMemory::Memcmp(ShaderConstants, ConstantCache.Constants, sizeof(uint32) * NumShaderConstants) != 0)
    {
        FMemory::Memcpy(ConstantCache.Constants, ShaderConstants, sizeof(uint32) * NumShaderConstants);
        ConstantCache.NumConstants = NumShaderConstants;
        
        GraphicsState.bBindShaderConstants = true;
        ComputeState.bBindShaderConstants  = true;
    }
}

void FD3D12CommandContextState::InternalSetRootSignature(FD3D12RootSignature* InRootSignature, EShaderVisibility::Type ShaderStage)
{
    if (ShaderStage == EShaderVisibility::All)
    {
        if (ComputeState.bBindRootSignature)
        {
            Context.GetCommandList()->SetComputeRootSignature(InRootSignature->GetD3D12RootSignature());
            ComputeState.bBindRootSignature = false;

            CommonState.ConstantBufferCache.DirtyDescriptorTableAll();
            CommonState.ShaderResourceViewCache.DirtyDescriptorTableAll();
            CommonState.UnorderedAccessViewCache.DirtyDescriptorTableAll();
            CommonState.SamplerStateCache.DirtyDescriptorTableAll();
        }
    }
    else
    {
        if (GraphicsState.bBindRootSignature)
        {
            Context.GetCommandList()->SetGraphicsRootSignature(InRootSignature->GetD3D12RootSignature());
            GraphicsState.bBindRootSignature = false;

            CommonState.ConstantBufferCache.DirtyDescriptorTableAll();
            CommonState.ShaderResourceViewCache.DirtyDescriptorTableAll();
            CommonState.UnorderedAccessViewCache.DirtyDescriptorTableAll();
            CommonState.SamplerStateCache.DirtyDescriptorTableAll();
        }
    }
}
