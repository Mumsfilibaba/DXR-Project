#include "D3D12CommandContextState.h"
#include "Core/Memory/Memory.h"

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
        D3D12_ERROR("Failed to initialize DescriptorCache");
        return false;
    }

    ResetState();
    return true;
}

void FD3D12CommandContextState::BindGraphicsStates()
{
    FD3D12RootSignature* RootSignture = GraphicsState.PipelineState->GetRootSignature();
    if (GraphicsState.bBindPipelineState)
    {
        Context.GetCommandList().SetPipelineState(GraphicsState.PipelineState->GetD3D12PipelineState());
        GraphicsState.bBindPipelineState = false;
    }

    D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = GraphicsState.PipelineState->GetD3D12PrimitiveTopology();
    if (GraphicsState.bBindPrimitiveTopology)
    {
        Context.GetCommandList().IASetPrimitiveTopology(PrimitiveTopology);
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

    if (GraphicsState.bBindShadingRateImage)
    {
        ID3D12Resource* Resource = GraphicsState.ShadingRateImage ? GraphicsState.ShadingRateImage->GetD3D12Resource()->GetD3D12Resource() : nullptr;
        Context.GetCommandList().RSSetShadingRateImage(Resource);
        GraphicsState.bBindShadingRateImage = false;
    }

    if (GraphicsState.bBindShadingRate)
    {
        D3D12_SHADING_RATE_COMBINER Combiners[] =
        {
            D3D12_SHADING_RATE_COMBINER_OVERRIDE,
            D3D12_SHADING_RATE_COMBINER_OVERRIDE,
        };

        Context.GetCommandList().RSSetShadingRate(GraphicsState.ShadingRate, Combiners);
        GraphicsState.bBindShadingRate = false;
    }

    BindResources(RootSignture, ShaderVisibility_Vertex, ShaderVisibility_Pixel, bRootSignatureReset);
    BindSamplers(RootSignture, ShaderVisibility_Vertex, ShaderVisibility_Pixel, bRootSignatureReset);

    if (ComputeState.bBindShaderConstants)
    {
        BindShaderConstants(RootSignture, ShaderVisibility_Pixel);
        ComputeState.bBindShaderConstants = false;
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
        Context.GetCommandList().RSSetViewports(GraphicsState.Viewports, GraphicsState.NumViewports);
        GraphicsState.bBindViewports = false;
    }

    if (GraphicsState.bBindScissorRects)
    {
        Context.GetCommandList().RSSetScissorRects(GraphicsState.ScissorRects, GraphicsState.NumScissorRects);
        GraphicsState.bBindScissorRects = false;
    }

    if (GraphicsState.bBindBlendFactor)
    {
        Context.GetCommandList().OMSetBlendFactor(GraphicsState.BlendFactor);
        GraphicsState.bBindBlendFactor = false;
    }
}

void FD3D12CommandContextState::BindComputeState()
{
    FD3D12RootSignature* RootSignture = ComputeState.PipelineState->GetRootSignature();
    if (ComputeState.bBindPipelineState)
    {
        Context.GetCommandList().SetPipelineState(ComputeState.PipelineState->GetD3D12PipelineState());
        ComputeState.bBindPipelineState = false;
    }

    bool bRootSignatureReset = false;
    if (ComputeState.bBindRootSignature)
    {
        bRootSignatureReset = InternalSetRootSignature(RootSignture, ShaderVisibility_All);
    }

    BindResources(RootSignture, ShaderVisibility_All, ShaderVisibility_All, bRootSignatureReset);
    BindSamplers(RootSignture, ShaderVisibility_All, ShaderVisibility_All, bRootSignatureReset);

    if (ComputeState.bBindShaderConstants)
    {
        BindShaderConstants(RootSignture, ShaderVisibility_All);
        ComputeState.bBindShaderConstants = false;
    }
}

void FD3D12CommandContextState::BindSamplers(FD3D12RootSignature* RootSignature, EShaderVisibility StartStage, EShaderVisibility EndStage, bool bForceBinding)
{
    const D3D12_RESOURCE_BINDING_TIER ResourceBindingTier = GetDevice()->GetResourceBindingTier();
    uint32 NumSamplers[ShaderVisibility_Count];

    constexpr int32 MaxTries = 4;
    uint32 NumSamplerDescriptors;
    for (int32 NumTries = 0; NumTries < MaxTries; NumTries++)
    {
        NumSamplerDescriptors = 0;
        for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
        {
            FShaderResourceRange& ResourceRange = CommonState.ShaderResourceCounts[CurrentStage];
            if (ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
            {
                NumSamplers[CurrentStage] = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_Sampler);
            }
            else
            {
                NumSamplers[CurrentStage] = ResourceRange.NumSamplers;
            }

            NumSamplerDescriptors += NumSamplers[CurrentStage];
        }

        if (!CommonState.DescriptorCache.GetSamplerHeap().HasSpace(NumSamplerDescriptors))
        {
            if (!CommonState.DescriptorCache.GetSamplerHeap().Realloc())
            {
                DEBUG_BREAK();
                return;
            }

            LOG_INFO("SamplerHeap Roll-Over");
        }
    }

    CommonState.DescriptorCache.SetDescriptorHeaps();

    const uint32 StartHandleOffset = CommonState.DescriptorCache.GetSamplerHeap().AllocateHandles(NumSamplerDescriptors);
    uint32 DescriptorHandleOffset = StartHandleOffset;
    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (bForceBinding || CommonState.SamplerStateCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.SetSamplers(CommonState.SamplerStateCache, RootSignature, CurrentStage, NumSamplers[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumSamplerDescriptors);
        }
    }

    CommonState.DescriptorCache.GetSamplerHeap().SetCurrentHandle(DescriptorHandleOffset);
}

void FD3D12CommandContextState::BindResources(FD3D12RootSignature* RootSignature, EShaderVisibility StartStage, EShaderVisibility EndStage, bool bForceBinding)
{
    const D3D12_RESOURCE_BINDING_TIER ResourceBindingTier = GetDevice()->GetResourceBindingTier();
    uint32 NumCBVs[ShaderVisibility_Count];
    uint32 NumSRVs[ShaderVisibility_Count];
    uint32 NumUAVs[ShaderVisibility_Count];

    // NOTE: In case any of the descriptor heaps "roll-over" and we allocate a new heap, we clear the dirty states and need to recalculate the necessary descriptors
    constexpr int32 MaxTries = 4;
    
    uint32 NumResourceDescriptors;
    for (int32 NumTries = 0; NumTries < MaxTries; NumTries++)
    {
        NumResourceDescriptors = 0;
        for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
        {
            FShaderResourceRange& ResourceRange = CommonState.ShaderResourceCounts[CurrentStage];
            if (ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
            {
                NumCBVs[CurrentStage] = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_CBV);
            }
            else
            {
                NumCBVs[CurrentStage] = ResourceRange.NumCBVs;
            }

            NumResourceDescriptors += NumCBVs[CurrentStage];

            if (ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
            {
                NumSRVs[CurrentStage] = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_SRV);
            }
            else
            {
                NumSRVs[CurrentStage] = ResourceRange.NumSRVs;
            }

            NumResourceDescriptors += NumSRVs[CurrentStage];

            if (ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
            {
                NumUAVs[CurrentStage] = RootSignature->GetMaxResourceCount(CurrentStage, ResourceType_UAV);
            }
            else
            {
                NumUAVs[CurrentStage] = ResourceRange.NumUAVs;
            }

            NumResourceDescriptors += NumUAVs[CurrentStage];
        }

        bool bDescriptorHeapRolledOver = false;
        if (!CommonState.DescriptorCache.GetResourceHeap().HasSpace(NumResourceDescriptors))
        {
            if (!CommonState.DescriptorCache.GetResourceHeap().Realloc())
            {
                LOG_INFO("ResourceHeap Roll-Over");
                DEBUG_BREAK();
                return;
            }

            bDescriptorHeapRolledOver = true;
        }

        // NOTE: If our DescriptorHeaps rolled over we want to finish up our current CommandList
        if (bDescriptorHeapRolledOver)
        {
            ResetStateResources();
            continue;
        }

        break;
    }

    CommonState.DescriptorCache.SetDescriptorHeaps();

    const uint32 StartHandleOffset = CommonState.DescriptorCache.GetResourceHeap().AllocateHandles(NumResourceDescriptors);
    uint32 DescriptorHandleOffset = StartHandleOffset;
    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (bForceBinding || CommonState.ConstantBufferCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.SetCBVs(CommonState.ConstantBufferCache, RootSignature, CurrentStage, NumCBVs[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
        }
    }

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (bForceBinding || CommonState.ShaderResourceViewCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.SetSRVs(CommonState.ShaderResourceViewCache, RootSignature, CurrentStage, NumSRVs[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
        }
    }

    for (EShaderVisibility CurrentStage = StartStage; CurrentStage <= EndStage; CurrentStage = EShaderVisibility(CurrentStage + 1))
    {
        if (bForceBinding || CommonState.UnorderedAccessViewCache.IsDirty(CurrentStage) || GD3D12ForceBinding)
        {
            CommonState.DescriptorCache.SetUAVs(CommonState.UnorderedAccessViewCache, RootSignature, CurrentStage, NumUAVs[CurrentStage], DescriptorHandleOffset);
            CHECK(DescriptorHandleOffset <= StartHandleOffset + NumResourceDescriptors);
        }
    }

    // If not all handles are used, return the remaining handles
    CommonState.DescriptorCache.GetResourceHeap().SetCurrentHandle(DescriptorHandleOffset);
}

void FD3D12CommandContextState::BindShaderConstants(FD3D12RootSignature* InRootSignature, EShaderVisibility ShaderStage)
{
    int32 ParameterIndex = InRootSignature->Get32BitConstantsIndex();
    if (ParameterIndex >= 0)
    {
        FD3D12ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache;
        if (ShaderStage == ShaderVisibility_All)
        {
            Context.GetCommandList().SetComputeRoot32BitConstants(ConstantCache.Constants, ConstantCache.NumConstants, 0, ParameterIndex);
        }
        else
        {
            Context.GetCommandList().SetGraphicsRoot32BitConstants(ConstantCache.Constants, ConstantCache.NumConstants, 0, ParameterIndex);
        }
    }
}

void FD3D12CommandContextState::ResetState()
{
    FMemory::Memzero(CommonState.ShaderResourceCounts, sizeof(CommonState.ShaderResourceCounts));
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
    
    GraphicsState.PipelineState    = nullptr;
    GraphicsState.ShadingRate      = D3D12_SHADING_RATE_1X1;
    GraphicsState.ShadingRateImage = nullptr;

    GraphicsState.bBindIndexBuffer       = true;
    GraphicsState.bBindRenderTargets     = true;
    GraphicsState.bBindBlendFactor       = true;
    GraphicsState.bBindPipelineState     = true;
    GraphicsState.bBindScissorRects      = true;
    GraphicsState.bBindViewports         = true;
    GraphicsState.bBindRootSignature     = true;
    GraphicsState.bBindShadingRate       = GD3D12SupportsShadingRate;
    GraphicsState.bBindShadingRateImage  = GD3D12SupportsShadingRateImage;
    GraphicsState.bBindVertexBuffers     = true;
    GraphicsState.bBindShaderConstants   = true;
    GraphicsState.bBindPrimitiveTopology = true;

    ComputeState.PipelineState        = nullptr;
    ComputeState.bBindPipelineState   = true;
    ComputeState.bBindRootSignature   = true;
    ComputeState.bBindShaderConstants = true;
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
    GraphicsState.bBindShadingRate       = GD3D12SupportsShadingRate;
    GraphicsState.bBindShadingRateImage  = GD3D12SupportsShadingRateImage;
    GraphicsState.bBindVertexBuffers     = true;
    GraphicsState.bBindShaderConstants   = true;
    GraphicsState.bBindPrimitiveTopology = true;

    ComputeState.bBindPipelineState   = true;
    ComputeState.bBindRootSignature   = true;
    ComputeState.bBindShaderConstants = true;
}

void FD3D12CommandContextState::SetGraphicsPipelineState(FD3D12GraphicsPipelineState* InGraphicsPipelineState)
{
    FD3D12GraphicsPipelineState* CurrentGraphicsPipelineState = GraphicsState.PipelineState.Get();
    if (CurrentGraphicsPipelineState != InGraphicsPipelineState)
    {
        FD3D12RootSignature* RootSignature        = InGraphicsPipelineState      ? InGraphicsPipelineState->GetRootSignature()      : nullptr;
        FD3D12RootSignature* CurrentRootSignature = CurrentGraphicsPipelineState ? CurrentGraphicsPipelineState->GetRootSignature() : nullptr;
        if (CurrentRootSignature != RootSignature)
        {
            GraphicsState.bBindRootSignature = true;
        }

        D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology        = InGraphicsPipelineState      ? InGraphicsPipelineState->GetD3D12PrimitiveTopology()      : D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        D3D12_PRIMITIVE_TOPOLOGY CurrentPrimitiveTopology = CurrentGraphicsPipelineState ? CurrentGraphicsPipelineState->GetD3D12PrimitiveTopology() : D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        if (CurrentPrimitiveTopology != PrimitiveTopology)
        {
            GraphicsState.bBindPrimitiveTopology = true;
        }

        if (InGraphicsPipelineState)
        {
            if (FD3D12VertexShader* VertexShader = InGraphicsPipelineState->GetVertexShader())
            {
                InternalSetShaderStageResourceCount(VertexShader, ShaderVisibility_Vertex);
            }
            if (FD3D12DomainShader* DomainShader = InGraphicsPipelineState->GetDomainShader())
            {
                InternalSetShaderStageResourceCount(DomainShader, ShaderVisibility_Domain);
            }
            if (FD3D12HullShader* HullShader = InGraphicsPipelineState->GetHullShader())
            {
                InternalSetShaderStageResourceCount(HullShader, ShaderVisibility_Hull);
            }
            if (FD3D12GeometryShader* GeometryShader = InGraphicsPipelineState->GetGeometryShader())
            {
                InternalSetShaderStageResourceCount(GeometryShader, ShaderVisibility_Geometry);
            }
            if (FD3D12PixelShader* PixelShader = InGraphicsPipelineState->GetPixelShader())
            {
                InternalSetShaderStageResourceCount(PixelShader, ShaderVisibility_Pixel);
            }
        }

        GraphicsState.PipelineState = MakeSharedRef<FD3D12GraphicsPipelineState>(InGraphicsPipelineState);
        GraphicsState.bBindPipelineState = true;
    }
}

void FD3D12CommandContextState::SetComputePipelineState(FD3D12ComputePipelineState* InComputePipelineState)
{
    FD3D12ComputePipelineState* CurrentComputePipelineState = ComputeState.PipelineState.Get();
    if (CurrentComputePipelineState != InComputePipelineState)
    {
        FD3D12RootSignature* RootSignature        = InComputePipelineState      ? InComputePipelineState->GetRootSignature()      : nullptr;
        FD3D12RootSignature* CurrentRootSignature = CurrentComputePipelineState ? CurrentComputePipelineState->GetRootSignature() : nullptr;
        if (CurrentRootSignature != RootSignature)
        {
            ComputeState.bBindRootSignature = true;
        }

        if (InComputePipelineState)
        {
            if (FD3D12ComputeShader* ComputeShader = InComputePipelineState->GetComputeShader())
            {
                InternalSetShaderStageResourceCount(ComputeShader, ShaderVisibility_All);
            }
        }

        ComputeState.PipelineState = MakeSharedRef<FD3D12ComputePipelineState>(InComputePipelineState);
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
            GraphicsState.bBindRenderTargets = true;
        }
    }
}

void FD3D12CommandContextState::SetShadingRate(EShadingRate ShadingRate)
{
    D3D12_SHADING_RATE D3DShadingRate = ConvertShadingRate(ShadingRate);
    if (GraphicsState.ShadingRate != D3DShadingRate)
    {
        GraphicsState.ShadingRate = D3DShadingRate;
        GraphicsState.bBindShadingRate = GD3D12SupportsShadingRate;
    }
}

void FD3D12CommandContextState::SetShadingRateImage(FD3D12Texture* ShadingRateImage)
{
    if (GraphicsState.ShadingRateImage != ShadingRateImage)
    {
        GraphicsState.ShadingRateImage = ShadingRateImage;
        GraphicsState.bBindShadingRateImage = GD3D12SupportsShadingRateImage;
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
        FD3D12Resource* Resource = VertexBuffer->GetD3D12Resource();
        CurrentVBV.BufferLocation = Resource->GetGPUVirtualAddress();
        CurrentVBV.SizeInBytes    = static_cast<uint32>(VertexBuffer->GetSize());
        CurrentVBV.StrideInBytes  = VertexBuffer->GetStride();
    }
    else
    {
        FMemory::Memzero(&CurrentVBV);
    }

    if (FMemory::Memcmp(&CurrentVBV, &GraphicsState.VBCache.VertexBuffers[VertexBufferSlot], sizeof(D3D12_VERTEX_BUFFER_VIEW)) != 0)
    {
        FMemory::Memcpy(&GraphicsState.VBCache.VertexBuffers[VertexBufferSlot], &CurrentVBV, sizeof(D3D12_VERTEX_BUFFER_VIEW));
        GraphicsState.VBCache.NumVertexBuffers = FMath::Max(GraphicsState.VBCache.NumVertexBuffers, VertexBufferSlot + 1);
        GraphicsState.bBindVertexBuffers = true;
    }
}

void FD3D12CommandContextState::SetIndexBuffer(FD3D12Buffer* IndexBuffer, DXGI_FORMAT IndexFormat)
{
    D3D12_INDEX_BUFFER_VIEW NewIndexBuffer;
    if (IndexBuffer)
    {
        FD3D12Resource* Resource = IndexBuffer->GetD3D12Resource();
        NewIndexBuffer.BufferLocation = Resource->GetGPUVirtualAddress();
        NewIndexBuffer.Format         = IndexFormat;
        NewIndexBuffer.SizeInBytes    = static_cast<uint32>(IndexBuffer->GetSize());
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
        CommonState.ShaderResourceViewCache.NumViews[ShaderStage] = FMath::Max<uint8>(CommonState.ShaderResourceViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ShaderResourceViewCache.bDirty[ShaderStage]   = true;
    }
}

void FD3D12CommandContextState::SetUAV(FD3D12UnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& UAVCache = CommonState.UnorderedAccessViewCache.ResourceViews[ShaderStage];
    if (UAVCache[ResourceIndex] != UnorderedAccessView)
    {
        UAVCache[ResourceIndex] = UnorderedAccessView;
        CommonState.UnorderedAccessViewCache.NumViews[ShaderStage] = FMath::Max<uint8>(CommonState.UnorderedAccessViewCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.UnorderedAccessViewCache.bDirty[ShaderStage]   = true;
    }
}

void FD3D12CommandContextState::SetCBV(FD3D12ConstantBufferView* ConstantBufferView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    auto& CBVCache = CommonState.ConstantBufferCache.ResourceViews[ShaderStage];
    if (CBVCache[ResourceIndex] != ConstantBufferView)
    {
        CBVCache[ResourceIndex] = ConstantBufferView;
        CommonState.ConstantBufferCache.NumBuffers[ShaderStage] = FMath::Max<uint8>(CommonState.ConstantBufferCache.NumBuffers[ShaderStage], static_cast<uint8>(ResourceIndex) + 1);
        CommonState.ConstantBufferCache.bDirty[ShaderStage]     = true;
    }
}

void FD3D12CommandContextState::SetSampler(FD3D12SamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex)
{
    auto& SamplerCache = CommonState.SamplerStateCache.SamplerStates[ShaderStage];
    if (SamplerCache[SamplerIndex] != SamplerState)
    {
        SamplerCache[SamplerIndex] = SamplerState;
        CommonState.SamplerStateCache.NumSamplers[ShaderStage] = FMath::Max<uint8>(CommonState.SamplerStateCache.NumSamplers[ShaderStage], static_cast<uint8>(SamplerIndex) + 1);
        CommonState.SamplerStateCache.bDirty[ShaderStage]      = true;
    }
}

void FD3D12CommandContextState::SetShaderConstants(const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    FD3D12ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache;
    if (NumShaderConstants != ConstantCache.NumConstants || FMemory::Memcmp(ShaderConstants, ConstantCache.Constants, sizeof(uint32) * NumShaderConstants) != 0)
    {
        FMemory::Memcpy(ConstantCache.Constants, ShaderConstants, sizeof(uint32) * NumShaderConstants);
        ConstantCache.NumConstants         = NumShaderConstants;
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
            Context.GetCommandList().SetComputeRootSignature(InRootSignature);
            ComputeState.bBindRootSignature = false;
            bRootSignatureReset = true;
        }
    }
    else
    {
        if (GraphicsState.bBindRootSignature)
        {
            Context.GetCommandList().SetGraphicsRootSignature(InRootSignature);
            GraphicsState.bBindRootSignature = false;
            bRootSignatureReset = true;
        }
    }

    return bRootSignatureReset;
}

void FD3D12CommandContextState::InternalSetShaderStageResourceCount(FD3D12Shader* Shader, EShaderVisibility ShaderStage)
{
    CHECK(Shader != nullptr);
    const FShaderResourceCount& ResourceCount = Shader->GetResourceCount();
    CommonState.ShaderResourceCounts[ShaderStage] = ResourceCount.Ranges;
}
