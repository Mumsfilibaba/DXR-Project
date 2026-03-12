#include "Core/Memory/Memory.h"
#include "D3D12RHI/D3D12CommandContextState.h"
#include "D3D12RHI/D3D12CommandContext.h"

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

void FD3D12CommandContextState::BindGraphicsStates()
{
    bool bCommandListSplit;
    
    do
    {
        bCommandListSplit = false;

        FD3D12RootSignature* RootSignture = GraphicsState.PipelineState->GetRootSignature();
        if (GraphicsState.bBindPipelineState)
        {
            Context.GetCommandList()->SetPipelineState(GraphicsState.PipelineState->GetD3D12PipelineState());
            GraphicsState.bBindPipelineState = false;
        }

        D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = GraphicsState.PipelineState->GetD3D12PrimitiveTopology();
        if (GraphicsState.bBindPrimitiveTopology)
        {
            Context.GetCommandList()->IASetPrimitiveTopology(PrimitiveTopology);
            GraphicsState.bBindPrimitiveTopology = false;
        }

        bool bRootSignatureReset = false;
        if (GraphicsState.bBindRootSignature)
        {
            bRootSignatureReset = InternalSetRootSignature(RootSignture, ShaderVisibility_Pixel);
        }

        if (GraphicsState.bBindRenderTargets)
        {
            CommonState.DescriptorCache.SetRenderTargets(GraphicsState.RTCache);
            GraphicsState.bBindRenderTargets = false;
        }

    #ifdef __ID3D12GraphicsCommandList5_INTERFACE_DEFINED__
        if (Context.GetCommandList().GetGraphicsCommandList5().IsValid())
        {
            if (GraphicsState.bBindShadingRateImage)
            {
                ID3D12Resource* Resource = GraphicsState.ShadingRateImage ? GraphicsState.ShadingRateImage->GetResource()->GetD3D12Resource() : nullptr;
                Context.GetCommandList().GetGraphicsCommandList5()->RSSetShadingRateImage(Resource);
                GraphicsState.bBindShadingRateImage = false;
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

        bCommandListSplit |= BindResources(RootSignture, GraphicsState.PipelineState.Get(), ShaderVisibility_Vertex, ShaderVisibility_Pixel, bRootSignatureReset);
        bCommandListSplit |= BindSamplers(RootSignture, GraphicsState.PipelineState.Get(), ShaderVisibility_Vertex, ShaderVisibility_Pixel, bRootSignatureReset);

    } while (bCommandListSplit);

    if (GraphicsState.bBindShaderConstants)
    {
        BindShaderConstants(GraphicsState.PipelineState->GetRootSignature(), ShaderVisibility_Pixel);
        GraphicsState.bBindShaderConstants = false;
    }

    if (GraphicsState.bBindVertexBuffers)
    {
        CommonState.DescriptorCache.SetVertexBuffers(GraphicsState.VBCache);
        GraphicsState.bBindVertexBuffers = false;
    }

    if (GraphicsState.bBindIndexBuffer)
    {
        CommonState.DescriptorCache.SetIndexBuffer(GraphicsState.IBCache);
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
}

void FD3D12CommandContextState::BindComputeState()
{
    bool bCommandListSplit;
    do
    {
        bCommandListSplit = false;

        FD3D12RootSignature* RootSignture = ComputeState.PipelineState->GetRootSignature();
        if (ComputeState.bBindPipelineState)
        {
            Context.GetCommandList()->SetPipelineState(ComputeState.PipelineState->GetD3D12PipelineState());
            ComputeState.bBindPipelineState = false;
        }

        bool bRootSignatureReset = false;
        if (ComputeState.bBindRootSignature)
        {
            bRootSignatureReset = InternalSetRootSignature(RootSignture, ShaderVisibility_All);
        }

        bCommandListSplit |= BindResources(RootSignture, ComputeState.PipelineState.Get(), ShaderVisibility_All, ShaderVisibility_All, bRootSignatureReset);
        bCommandListSplit |= BindSamplers(RootSignture, ComputeState.PipelineState.Get(), ShaderVisibility_All, ShaderVisibility_All, bRootSignatureReset);

    } while (bCommandListSplit);

    if (ComputeState.bBindShaderConstants)
    {
        BindShaderConstants(ComputeState.PipelineState->GetRootSignature(), ShaderVisibility_All);
        ComputeState.bBindShaderConstants = false;
    }
}

bool FD3D12CommandContextState::BindSamplers(FD3D12RootSignature* RootSignature, FD3D12PipelineState* PipelineState, EShaderVisibility StartStage, EShaderVisibility EndStage, bool bForceBinding)
{
    uint32 NumSamplers[ShaderVisibility_Count];

    constexpr int32 MaxTries = 4;
    bool bCommandListSplit = false;

    uint32 NumSamplerDescriptors;
    for (int32 NumTries = 0; NumTries < MaxTries; NumTries++)
    {
        NumSamplerDescriptors = 0;

        for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
        {
            const uint32 MaxSamplers = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_Sampler);
            if (GD3D12ResourceBindingTier > D3D12_RESOURCE_BINDING_TIER_1)
            {
                NumSamplers[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, ResourceType_Sampler);
            }
            else
            {
                NumSamplers[CurrentStage] = MaxSamplers;
            }

            NumSamplerDescriptors += NumSamplers[CurrentStage];
        }

        if (!CommonState.DescriptorCache.GetSamplerHeap().HasSpace(NumSamplerDescriptors))
        {
            if (!CommonState.DescriptorCache.GetSamplerHeap().Realloc())
            {
                LOG_WARNING("SamplerHeap exhausted, splitting CommandList to recycle blocks");

                Context.SplitCommandListForDescriptorHeapRollover();
                bCommandListSplit = true;

                if (!CommonState.DescriptorCache.GetSamplerHeap().Realloc())
                {
                    D3D12_ERROR("Failed to allocate sampler descriptor block after CommandList split");
                    return bCommandListSplit;
                }
            }

            CommonState.DescriptorCache.InvalidateCachedSamplerTables();
            CommonState.SamplerStateCache.DirtyStateAll();

            LOG_WARNING("SamplerHeap Roll-Over");
            continue;
        }

        break;
    }

    CommonState.DescriptorCache.SetDescriptorHeaps();

    const uint32 StartHandleOffset = CommonState.DescriptorCache.GetSamplerHeap().AllocateHandles(NumSamplerDescriptors);
    uint32 DescriptorHandleOffset  = StartHandleOffset;

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (!NumSamplers[CurrentStage])
        {
            continue;
        }

        if (bForceBinding || CommonState.SamplerStateCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.SetSamplers(CommonState.SamplerStateCache, RootSignature, CurrentStage, NumSamplers[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumSamplerDescriptors);
        }
    }

    CommonState.DescriptorCache.GetSamplerHeap().SetCurrentHandle(DescriptorHandleOffset);
    return bCommandListSplit;
}

bool FD3D12CommandContextState::BindResources(FD3D12RootSignature* RootSignature, FD3D12PipelineState* PipelineState, EShaderVisibility StartStage, EShaderVisibility EndStage, bool bForceBinding)
{
    uint32 NumCBVs[ShaderVisibility_Count];
    uint32 NumSRVs[ShaderVisibility_Count];
    uint32 NumUAVs[ShaderVisibility_Count];

    constexpr int32 MaxTries = 4;
    bool bCommandListSplit = false;

    uint32 NumResourceDescriptors;
    for (int32 NumTries = 0; NumTries < MaxTries; NumTries++)
    {
        NumResourceDescriptors = 0;

        for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
        {
            const uint32 MaxCBVs = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_CBV);
            const uint32 MaxSRVs = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_SRV);
            const uint32 MaxUAVs = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_UAV);

            if (GD3D12ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3)
            {
                NumCBVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, ResourceType_CBV);
                NumSRVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, ResourceType_SRV);
                NumUAVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, ResourceType_UAV);
            }
            else if (GD3D12ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2)
            {
                NumCBVs[CurrentStage] = MaxCBVs;
                NumSRVs[CurrentStage] = PipelineState->GetEffectiveDescriptorCount(CurrentStage, ResourceType_SRV);
                NumUAVs[CurrentStage] = MaxUAVs;
            }
            else
            {
                NumCBVs[CurrentStage] = MaxCBVs;
                NumSRVs[CurrentStage] = MaxSRVs;
                NumUAVs[CurrentStage] = MaxUAVs;
            }

            NumResourceDescriptors += NumCBVs[CurrentStage];
            NumResourceDescriptors += NumSRVs[CurrentStage];
            NumResourceDescriptors += NumUAVs[CurrentStage];
        }

        bool bDescriptorHeapRolledOver = false;
        if (!CommonState.DescriptorCache.GetResourceHeap().HasSpace(NumResourceDescriptors))
        {
            if (!CommonState.DescriptorCache.GetResourceHeap().Realloc())
            {
                LOG_WARNING("ResourceHeap exhausted, splitting CommandList to recycle blocks");

                Context.SplitCommandListForDescriptorHeapRollover();
                bCommandListSplit = true;

                if (!CommonState.DescriptorCache.GetResourceHeap().Realloc())
                {
                    D3D12_ERROR("Failed to allocate resource descriptor block after CommandList split");
                    return bCommandListSplit;
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

    CommonState.DescriptorCache.SetDescriptorHeaps();

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (!CommonState.ConstantBufferCache.IsDirty(CurrentStage) && NumCBVs[CurrentStage] > 0)
        {
            const FD3D12DescriptorTableMapping& CBVMapping = RootSignature->GetDescriptorTableMapping(CurrentStage, ResourceType_CBV);

            auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[CurrentStage];
            for (uint32 Slot = 0; Slot < NumCBVs[CurrentStage]; Slot++)
            {
                const uint16 Register = CBVMapping.GetRegisterForSlot(static_cast<uint8>(Slot));
                if (FD3D12Buffer* Buffer = CBVCache[Register])
                {
                    if (FD3D12ConstantBufferView* View = Buffer->GetOrCreateConstantBufferView())
                    {
                        if (View->GetDescriptorVersion() != CommonState.ConstantBufferCache.ViewVersions[CurrentStage][Register])
                        {
                            CommonState.ConstantBufferCache.ViewVersions[CurrentStage][Register] = View->GetDescriptorVersion();
                            CommonState.ConstantBufferCache.bDirty[CurrentStage]                 = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!CommonState.ShaderResourceViewCache.IsDirty(CurrentStage) && NumSRVs[CurrentStage] > 0)
        {
            const FD3D12DescriptorTableMapping& SRVMapping = RootSignature->GetDescriptorTableMapping(CurrentStage, ResourceType_SRV);

            auto& SRVCache = CommonState.ShaderResourceViewCache.ResourceViews[CurrentStage];
            for (uint32 Slot = 0; Slot < NumSRVs[CurrentStage]; Slot++)
            {
                const uint16 Register = SRVMapping.GetRegisterForSlot(static_cast<uint8>(Slot));
                if (FD3D12ShaderResourceView* View = SRVCache[Register])
                {
                    if (View->GetDescriptorVersion() != CommonState.ShaderResourceViewCache.ViewVersions[CurrentStage][Register])
                    {
                        CommonState.ShaderResourceViewCache.ViewVersions[CurrentStage][Register] = View->GetDescriptorVersion();
                        CommonState.ShaderResourceViewCache.bDirty[CurrentStage]                 = true;
                        break;
                    }
                }
            }
        }

        if (!CommonState.UnorderedAccessViewCache.IsDirty(CurrentStage) && NumUAVs[CurrentStage] > 0)
        {
            const FD3D12DescriptorTableMapping& UAVMapping = RootSignature->GetDescriptorTableMapping(CurrentStage, ResourceType_UAV);

            auto& UAVCache = CommonState.UnorderedAccessViewCache.ResourceViews[CurrentStage];
            for (uint32 Slot = 0; Slot < NumUAVs[CurrentStage]; Slot++)
            {
                const uint16 Register = UAVMapping.GetRegisterForSlot(static_cast<uint8>(Slot));
                if (FD3D12UnorderedAccessView* View = UAVCache[Register])
                {
                    if (View->GetDescriptorVersion() != CommonState.UnorderedAccessViewCache.ViewVersions[CurrentStage][Register])
                    {
                        CommonState.UnorderedAccessViewCache.ViewVersions[CurrentStage][Register] = View->GetDescriptorVersion();
                        CommonState.UnorderedAccessViewCache.bDirty[CurrentStage]                 = true;
                        break;
                    }
                }
            }
        }
    }

    const uint32 StartHandleOffset = CommonState.DescriptorCache.GetResourceHeap().AllocateHandles(NumResourceDescriptors);
    uint32 DescriptorHandleOffset  = StartHandleOffset;

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (NumCBVs[CurrentStage] > 0)
        {
            if (bForceBinding || CommonState.ConstantBufferCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
            {
                CommonState.DescriptorCache.SetCBVs(CommonState.ConstantBufferCache, RootSignature, CurrentStage, NumCBVs[CurrentStage], DescriptorHandleOffset);
                CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
            }
        }

        const FD3D12ShaderStage& Stage = RootSignature->GetShaderStage(CurrentStage);
        if (Stage.GetNumRootCBVs() > 0 && (bForceBinding || CommonState.ConstantBufferCache.IsDirty(CurrentStage) || GD3D12ForceBinding))
        {
            auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[CurrentStage];
            for (uint8 RootCBVIdx = 0; RootCBVIdx < Stage.GetNumRootCBVs(); RootCBVIdx++)
            {
                const int8   ParamIndex = Stage.GetRootCBVParameterIndexBySlot(RootCBVIdx);
                const uint16 Register   = Stage.GetRootCBVRegister(RootCBVIdx);
                CHECK(Register < D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);

                D3D12_GPU_VIRTUAL_ADDRESS GpuVA = 0;
                if (FD3D12Buffer* Buffer = CBVCache[Register])
                {
                    GpuVA = Buffer->GetGpuVirtualAddress();
                }

                if (CurrentStage == ShaderVisibility_All)
                {
                    Context.GetCommandList()->SetComputeRootConstantBufferView(ParamIndex, GpuVA);
                }
                else
                {
                    Context.GetCommandList()->SetGraphicsRootConstantBufferView(ParamIndex, GpuVA);
                }
            }
        }
    }

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (!NumSRVs[CurrentStage])
        {
            continue;
        }

        if (bForceBinding || CommonState.ShaderResourceViewCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.SetSRVs(CommonState.ShaderResourceViewCache, RootSignature, CurrentStage, NumSRVs[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
        }
    }

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (!NumUAVs[CurrentStage])
        {
            continue;
        }

        if (bForceBinding || CommonState.UnorderedAccessViewCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.SetUAVs(CommonState.UnorderedAccessViewCache, RootSignature, CurrentStage, NumUAVs[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
        }
    }

    // If not all handles are used, return the remaining handles
    CommonState.DescriptorCache.GetResourceHeap().SetCurrentHandle(DescriptorHandleOffset);
    return bCommandListSplit;
}

void FD3D12CommandContextState::BindShaderConstants(FD3D12RootSignature* InRootSignature, EShaderVisibility ShaderStage)
{
    int32 ParameterIndex = InRootSignature->Get32BitConstantsIndex();
    if (ParameterIndex >= 0)
    {
        FD3D12ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache;
        if (ShaderStage == ShaderVisibility_All)
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

    GraphicsState.RTCache.Clear();
    GraphicsState.VBCache.Clear();
    GraphicsState.IBCache.Clear();

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

    CommonState.ConstantBufferCache.DirtyStateAll();
    CommonState.ShaderResourceViewCache.DirtyStateAll();
    CommonState.UnorderedAccessViewCache.DirtyStateAll();
}

void FD3D12CommandContextState::ResetStateForNewCommandList()
{
    CommonState.DescriptorCache.DirtyDescriptorHeaps();
    CommonState.DescriptorCache.DirtyStateResources();

    CommonState.ConstantBufferCache.DirtyStateAll();
    CommonState.ShaderResourceViewCache.DirtyStateAll();
    CommonState.UnorderedAccessViewCache.DirtyStateAll();
    CommonState.SamplerStateCache.DirtyStateAll();

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

    ComputeState.bBindPipelineState      = true;
    ComputeState.bBindRootSignature      = true;
    ComputeState.bBindShaderConstants    = true;
}

void FD3D12CommandContextState::SetGraphicsPipelineState(FD3D12GraphicsPipelineState* InGraphicsPipelineState)
{
    FD3D12GraphicsPipelineState* CurrentGraphicsPipelineState = GraphicsState.PipelineState.Get();
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

        GraphicsState.PipelineState      = MakeSharedRef<FD3D12GraphicsPipelineState>(InGraphicsPipelineState);
        GraphicsState.bBindPipelineState = true;
    }
}

void FD3D12CommandContextState::SetComputePipelineState(FD3D12ComputePipelineState* InComputePipelineState)
{
    FD3D12ComputePipelineState* CurrentComputePipelineState = ComputeState.PipelineState.Get();
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

        ComputeState.PipelineState      = MakeSharedRef<FD3D12ComputePipelineState>(InComputePipelineState);
        ComputeState.bBindPipelineState = true;
    }
}

void FD3D12CommandContextState::SetRenderTargets(FD3D12RenderTargetView* const* RenderTargets, uint32 NumRenderTargets, FD3D12DepthStencilView* DepthStencil)
{
    if (GraphicsState.RTCache.DepthStencilView != DepthStencil)
    {
        GraphicsState.RTCache.DepthStencilView = DepthStencil;
        GraphicsState.bBindRenderTargets = true;
    }

    CHECK(NumRenderTargets < D3D12_MAX_RENDER_TARGET_COUNT);
    GraphicsState.RTCache.NumRenderTargets = NumRenderTargets;

    for (uint32 Index = 0; Index < NumRenderTargets; Index++)
    {
        if (GraphicsState.RTCache.RenderTargetViews[Index] != RenderTargets[Index])
        {
            GraphicsState.RTCache.RenderTargetViews[Index] = RenderTargets[Index];
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

void FD3D12CommandContextState::SetShadingRateImage(FD3D12Texture* ShadingRateImage)
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

void FD3D12CommandContextState::SetVertexBuffer(FD3D12Buffer* VertexBuffer, uint32 VertexBufferSlot)
{
    CHECK(VertexBufferSlot < D3D12_MAX_VERTEX_BUFFER_SLOTS);
    
    D3D12_VERTEX_BUFFER_VIEW CurrentVBV;
    if (VertexBuffer)
    {
        CurrentVBV.BufferLocation = VertexBuffer->GetGpuVirtualAddress();
        CurrentVBV.SizeInBytes    = static_cast<uint32>(VertexBuffer->GetInfo().Size);
        CurrentVBV.StrideInBytes  = VertexBuffer->GetInfo().Stride;

        Context.GetCommandList().UpdateResidency(VertexBuffer->GetResource()->GetResidencyHandle());
    }
    else
    {
        FMemory::Memzero(&CurrentVBV);
    }

    if (FMemory::Memcmp(&CurrentVBV, &GraphicsState.VBCache.VertexBuffers[VertexBufferSlot], sizeof(D3D12_VERTEX_BUFFER_VIEW)) != 0)
    {
        FMemory::Memcpy(&GraphicsState.VBCache.VertexBuffers[VertexBufferSlot], &CurrentVBV, sizeof(D3D12_VERTEX_BUFFER_VIEW));

        const uint8 NumVertexBuffers =  Math::Max(GraphicsState.VBCache.NumVertexBuffers, VertexBufferSlot + 1);
        GraphicsState.VBCache.NumVertexBuffers = NumVertexBuffers;
        GraphicsState.bBindVertexBuffers       = true;
    }
}

void FD3D12CommandContextState::SetIndexBuffer(FD3D12Buffer* IndexBuffer, DXGI_FORMAT IndexFormat)
{
    D3D12_INDEX_BUFFER_VIEW NewIndexBuffer;
    if (IndexBuffer)
    {
        NewIndexBuffer.BufferLocation = IndexBuffer->GetGpuVirtualAddress();
        NewIndexBuffer.Format         = IndexFormat;
        NewIndexBuffer.SizeInBytes    = static_cast<uint32>(IndexBuffer->GetInfo().Size);

        Context.GetCommandList().UpdateResidency(IndexBuffer->GetResource()->GetResidencyHandle());
    }
    else
    {
        FMemory::Memzero(&NewIndexBuffer);
    }

    if (FMemory::Memcmp(&NewIndexBuffer, &GraphicsState.IBCache.IndexBuffer, sizeof(D3D12_INDEX_BUFFER_VIEW)) != 0)
    {
        FMemory::Memcpy(&GraphicsState.IBCache.IndexBuffer, &NewIndexBuffer, sizeof(D3D12_INDEX_BUFFER_VIEW));
        GraphicsState.bBindIndexBuffer = true;
    }
}

void FD3D12CommandContextState::SetSRV(FD3D12ShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& SRVCache = CommonState.ShaderResourceViewCache.ResourceViews[ShaderStage];
    if (SRVCache[ResourceIndex] != ShaderResourceView)
    {
        SRVCache[ResourceIndex] = ShaderResourceView;

        const uint8 NumViews = Math::Max<uint8>(CommonState.ShaderResourceViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ShaderResourceViewCache.ViewVersions[ShaderStage][ResourceIndex] = ShaderResourceView ? ShaderResourceView->GetDescriptorVersion() : 0;
        CommonState.ShaderResourceViewCache.NumViews[ShaderStage]                    = NumViews;
        CommonState.ShaderResourceViewCache.bDirty[ShaderStage]                      = true;
    }
}

void FD3D12CommandContextState::SetUAV(FD3D12UnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& UAVCache = CommonState.UnorderedAccessViewCache.ResourceViews[ShaderStage];
    if (UAVCache[ResourceIndex] != UnorderedAccessView)
    {
        UAVCache[ResourceIndex] = UnorderedAccessView;

        const uint8 NumViews = Math::Max<uint8>(CommonState.UnorderedAccessViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.UnorderedAccessViewCache.ViewVersions[ShaderStage][ResourceIndex] = UnorderedAccessView ? UnorderedAccessView->GetDescriptorVersion() : 0;
        CommonState.UnorderedAccessViewCache.NumViews[ShaderStage]                    = NumViews;
        CommonState.UnorderedAccessViewCache.bDirty[ShaderStage]                      = true;
    }
}

void FD3D12CommandContextState::SetCBV(FD3D12Buffer* Buffer, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[ShaderStage];
    if (CBVCache[ResourceIndex] != Buffer)
    {
        CBVCache[ResourceIndex] = Buffer;

        const uint8 NumBuffers = Math::Max<uint8>(CommonState.ConstantBufferCache.NumBuffers[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ConstantBufferCache.ViewVersions[ShaderStage][ResourceIndex] = 0;
        CommonState.ConstantBufferCache.NumBuffers[ShaderStage]                  = NumBuffers;
        CommonState.ConstantBufferCache.bDirty[ShaderStage]                      = true;
    }
}

void FD3D12CommandContextState::SetSampler(FD3D12SamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex)
{
    auto& SamplerCache = CommonState.SamplerStateCache.SamplerStates[ShaderStage];
    if (SamplerCache[SamplerIndex] != SamplerState)
    {
        SamplerCache[SamplerIndex] = SamplerState;

        const uint8 NumSamplers = Math::Max<uint8>(CommonState.SamplerStateCache.NumSamplers[ShaderStage], static_cast<uint8>(SamplerIndex) + 1);
        CommonState.SamplerStateCache.NumSamplers[ShaderStage] = NumSamplers;
        CommonState.SamplerStateCache.bDirty[ShaderStage]      = true;
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

bool FD3D12CommandContextState::InternalSetRootSignature(FD3D12RootSignature* InRootSignature, EShaderVisibility ShaderStage)
{
    bool bRootSignatureReset = false;
    if (ShaderStage == ShaderVisibility_All)
    {
        if (ComputeState.bBindRootSignature)
        {
            Context.GetCommandList()->SetComputeRootSignature(InRootSignature->GetD3D12RootSignature());
            
            ComputeState.bBindRootSignature = false;
            bRootSignatureReset             = true;
        }
    }
    else
    {
        if (GraphicsState.bBindRootSignature)
        {
            Context.GetCommandList()->SetGraphicsRootSignature(InRootSignature->GetD3D12RootSignature());
            
            GraphicsState.bBindRootSignature = false;
            bRootSignatureReset              = true;
        }
    }

    return bRootSignatureReset;
}

