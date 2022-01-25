#include "D3D12Device.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandList.h"
#include "D3D12Core.h"
#include "D3D12Shader.h"
#include "D3D12RHIInterface.h"
#include "D3D12RHIBuffer.h"
#include "D3D12RHITexture.h"
#include "D3D12RHIPipelineState.h"
#include "D3D12RHIRayTracing.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12RHITimestampQuery.h"
#include "D3D12RHICommandContext.h"
#include "D3D12ResourceCast.inl"
#include "D3D12FunctionPointers.h"

#include "Core/Math/Vector2.h"
#include "Core/Debug/Profiler/FrameProfiler.h"

#include <pix.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ResourceBarrierBatcher

void CD3D12ResourceBarrierBatcher::AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
    Assert(Resource != nullptr);

    if (BeforeState != AfterState)
    {
        // Make sure we are not already have transition for this resource
        for (TArray<D3D12_RESOURCE_BARRIER>::IteratorType It = Barriers.StartIterator(); It != Barriers.EndIterator(); It++)
        {
            if ((*It).Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
            {
                if ((*It).Transition.pResource == Resource)
                {
                    if ((*It).Transition.StateBefore == AfterState)
                    {
                        It = Barriers.RemoveAt(It);
                    }
                    else
                    {
                        (*It).Transition.StateAfter = AfterState;
                    }

                    return;
                }
            }
        }

        // Add new resource barrier
        D3D12_RESOURCE_BARRIER Barrier;
        CMemory::Memzero(&Barrier);

        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource = Resource;
        Barrier.Transition.StateAfter = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        Barriers.Emplace(Barrier);
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12GPUResourceUploader

CD3D12GPUResourceUploader::CD3D12GPUResourceUploader(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
    , MappedMemory(nullptr)
    , SizeInBytes(0)
    , OffsetInBytes(0)
    , Resource(nullptr)
    , GarbageResources()
{
}

bool CD3D12GPUResourceUploader::Reserve(uint32 InSizeInBytes)
{
    if (InSizeInBytes == SizeInBytes)
    {
        return true;
    }

    if (Resource)
    {
        Resource->Unmap(0, nullptr);
        GarbageResources.Emplace(Resource);
        Resource.Reset();
    }

    D3D12_HEAP_PROPERTIES HeapProperties;
    CMemory::Memzero(&HeapProperties);

    HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    Desc.Format = DXGI_FORMAT_UNKNOWN;
    Desc.Width = InSizeInBytes;
    Desc.Height = 1;
    Desc.DepthOrArraySize = 1;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;

    HRESULT Result = GetDevice()->CreateCommitedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&Resource));
    if (SUCCEEDED(Result))
    {
        Resource->SetName(L"[D3D12GPUResourceUploader] Buffer");
        Resource->Map(0, nullptr, reinterpret_cast<void**>(&MappedMemory));

        SizeInBytes = InSizeInBytes;
        OffsetInBytes = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void CD3D12GPUResourceUploader::Reset()
{
    constexpr uint32 MAX_RESERVED_GARBAGE_RESOURCES = 5;
    constexpr uint32 NEW_RESERVED_GARBAGE_RESOURCES = 2;

    // Clear garbage resource, and release memory we do not need
    GarbageResources.Clear();
    if (GarbageResources.Capacity() >= MAX_RESERVED_GARBAGE_RESOURCES)
    {
        GarbageResources.Reserve(NEW_RESERVED_GARBAGE_RESOURCES);
    }

    // Reset memory offset
    OffsetInBytes = 0;
}

SD3D12UploadAllocation CD3D12GPUResourceUploader::LinearAllocate(uint32 InSizeInBytes)
{
    constexpr uint32 EXTRA_BYTES_ALLOCATED = 1024;

    const uint32 NeededSize = OffsetInBytes + InSizeInBytes;
    if (NeededSize > SizeInBytes)
    {
        Reserve(NeededSize + EXTRA_BYTES_ALLOCATED);
    }

    SD3D12UploadAllocation Allocation;
    Allocation.MappedPtr = MappedMemory + OffsetInBytes;
    Allocation.ResourceOffset = OffsetInBytes;
    OffsetInBytes += InSizeInBytes;
    return Allocation;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12CommandBatch

CD3D12CommandBatch::CD3D12CommandBatch(CD3D12Device* InDevice)
    : Device(InDevice)
    , CmdAllocator(InDevice)
    , GpuResourceUploader(InDevice)
    , OnlineResourceDescriptorHeap(nullptr)
    , OnlineSamplerDescriptorHeap(nullptr)
    , Resources()
{
}

bool CD3D12CommandBatch::Init()
{
    // TODO: Do not have D3D12_COMMAND_LIST_TYPE_DIRECT directly
    if (!CmdAllocator.Init(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    OnlineResourceDescriptorHeap = dbg_new CD3D12OnlineDescriptorHeap(Device, D3D12_DEFAULT_ONLINE_RESOURCE_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!OnlineResourceDescriptorHeap->Init())
    {
        return false;
    }

    OnlineSamplerDescriptorHeap = dbg_new CD3D12OnlineDescriptorHeap(Device, D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!OnlineSamplerDescriptorHeap->Init())
    {
        return false;
    }

    GpuResourceUploader.Reserve(1024);
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHICommandContext

CD3D12RHICommandContext* CD3D12RHICommandContext::Make(CD3D12Device* InDevice)
{
    TSharedRef<CD3D12RHICommandContext> NewContext = dbg_new CD3D12RHICommandContext(InDevice);
    if (NewContext && NewContext->Init())
    {
        return NewContext.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CD3D12RHICommandContext::CD3D12RHICommandContext(CD3D12Device* InDevice)
    : IRHICommandContext()
    , CD3D12DeviceChild(InDevice)
    , CmdQueue(InDevice)
    , CmdList(InDevice)
    , Fence(InDevice)
    , DescriptorCache(InDevice)
    , CmdBatches()
    , BarrierBatcher()
{
}

CD3D12RHICommandContext::~CD3D12RHICommandContext()
{
    Flush();
}

bool CD3D12RHICommandContext::Init()
{
    if (!CmdQueue.Init(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    // TODO: Have support for more than 6 commandbatches?
    for (uint32 i = 0; i < D3D12_NUM_BACK_BUFFERS; i++)
    {
        CD3D12CommandBatch& Batch = CmdBatches.Emplace(GetDevice());
        if (!Batch.Init())
        {
            D3D12_ERROR_ALWAYS("Failed to initialize D3D12CommandBatch");
            return false;
        }
    }

    if (!CmdList.Init(D3D12_COMMAND_LIST_TYPE_DIRECT, CmdBatches[0].GetCommandAllocator(), nullptr))
    {
        D3D12_ERROR_ALWAYS("Failed to initialize CommandList");
        return false;
    }

    FenceValue = 0;
    if (!Fence.Init(FenceValue))
    {
        D3D12_ERROR_ALWAYS("Failed to initialize Fence");
        return false;
    }

    if (!DescriptorCache.Init())
    {
        D3D12_ERROR_ALWAYS("Failed to initialize DescriptorCache");
        return false;
    }

    return true;
}

void CD3D12RHICommandContext::UpdateBuffer(CD3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    D3D12_ERROR(Resource != nullptr, "Resource cannot be nullptr");

    if (SizeInBytes)
    {
        D3D12_ERROR(SourceData != nullptr, "SourceData cannot be nullptr ");

        FlushResourceBarriers();

        CD3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();

        SD3D12UploadAllocation Allocation = GpuResourceUploader.LinearAllocate((uint32)SizeInBytes);
        CMemory::Memcpy(Allocation.MappedPtr, SourceData, SizeInBytes);

        CmdList.CopyBufferRegion(Resource->GetResource(), OffsetInBytes, GpuResourceUploader.GetGpuResource(), Allocation.ResourceOffset, SizeInBytes);

        CmdBatch->AddInUseResource(Resource);
    }
}

void CD3D12RHICommandContext::Begin()
{
    Assert(bIsReady == false);

    TRACE_FUNCTION_SCOPE();

    CmdBatch = &CmdBatches[NextCmdBatch];
    NextCmdBatch = (NextCmdBatch + 1) % CmdBatches.Size();

    // TODO: Investigate better ways of doing this 
    if (FenceValue >= CmdBatches.Size())
    {
        const uint64 WaitValue = NMath::Max<uint64>(FenceValue - (CmdBatches.Size() - 1), 0);
        Fence.WaitForValue(WaitValue);
    }

    if (!CmdBatch->Reset())
    {
        D3D12_ERROR_ALWAYS("Failed to reset D3D12CommandBatch");
        return;
    }

    InternalClearState();

    if (!CmdList.Reset(CmdBatch->GetCommandAllocator()))
    {
        D3D12_ERROR_ALWAYS("Failed to reset Commandlist");
        return;
    }

    bIsReady = true;
}

void CD3D12RHICommandContext::End()
{
    Assert(bIsReady == true);

    TRACE_FUNCTION_SCOPE();

    FlushResourceBarriers();

    CmdBatch = nullptr;

    const uint64 NewFenceValue = ++FenceValue;
    for (int32 i = 0; i < ResolveProfilers.Size(); i++)
    {
        ResolveProfilers[i]->ResolveQueries(*this);
    }

    ResolveProfilers.Clear();

    // Execute
    if (!CmdList.Close())
    {
        D3D12_ERROR_ALWAYS("Failed to close CommandList");
        return;
    }

    CmdQueue.ExecuteCommandList(&CmdList);

    if (!CmdQueue.SignalFence(Fence, NewFenceValue))
    {
        D3D12_ERROR_ALWAYS("Failed to signal Fence on the GPU");
        return;
    }

    bIsReady = false;
}

void CD3D12RHICommandContext::BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)
{
    CD3D12RHITimestampQuery* DxTimestampQuery = static_cast<CD3D12RHITimestampQuery*>(TimestampQuery);
    D3D12_ERROR(DxTimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
    DxTimestampQuery->BeginQuery(DxCmdList, Index);

    ResolveProfilers.Emplace(MakeSharedRef<CD3D12RHITimestampQuery>(DxTimestampQuery));
}

void CD3D12RHICommandContext::EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)
{
    CD3D12RHITimestampQuery* DxTimestampQuery = static_cast<CD3D12RHITimestampQuery*>(TimestampQuery);
    D3D12_ERROR(DxTimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
    DxTimestampQuery->EndQuery(DxCmdList, Index);
}

void CD3D12RHICommandContext::ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor)
{
    D3D12_ERROR(RenderTargetView != nullptr, "RenderTargetView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    CD3D12RHIRenderTargetView* DxRenderTargetView = static_cast<CD3D12RHIRenderTargetView*>(RenderTargetView);
    CmdBatch->AddInUseResource(DxRenderTargetView);

    CmdList.ClearRenderTargetView(DxRenderTargetView->GetOfflineHandle(), ClearColor.Elements, 0, nullptr);
}

void CD3D12RHICommandContext::ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue)
{
    D3D12_ERROR(DepthStencilView != nullptr, "DepthStencilView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    CD3D12RHIDepthStencilView* DxDepthStencilView = static_cast<CD3D12RHIDepthStencilView*>(DepthStencilView);
    CmdBatch->AddInUseResource(DxDepthStencilView);

    CmdList.ClearDepthStencilView(DxDepthStencilView->GetOfflineHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, ClearValue.Depth, ClearValue.Stencil);
}

void CD3D12RHICommandContext::ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor)
{
    D3D12_ERROR(UnorderedAccessView != nullptr, "UnorderedAccessView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    CD3D12RHIUnorderedAccessView* DxUnorderedAccessView = static_cast<CD3D12RHIUnorderedAccessView*>(UnorderedAccessView);
    CmdBatch->AddInUseResource(DxUnorderedAccessView);

    CD3D12OnlineDescriptorHeap* OnlineDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    const uint32 OnlineDescriptorHandleIndex = OnlineDescriptorHeap->AllocateHandles(1);

    const D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle = DxUnorderedAccessView->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle_CPU = OnlineDescriptorHeap->GetCPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    GetDevice()->CopyDescriptorsSimple(1, OnlineHandle_CPU, OfflineHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandle_GPU = OnlineDescriptorHeap->GetGPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    CmdList.ClearUnorderedAccessViewFloat(OnlineHandle_GPU, DxUnorderedAccessView, ClearColor.Elements);
}

void CD3D12RHICommandContext::SetShadingRate(EShadingRate ShadingRate)
{
    D3D12_SHADING_RATE DxShadingRate = ConvertShadingRate(ShadingRate);

    D3D12_SHADING_RATE_COMBINER Combiners[] =
    {
        D3D12_SHADING_RATE_COMBINER_OVERRIDE,
        D3D12_SHADING_RATE_COMBINER_OVERRIDE,
    };

    CmdList.RSSetShadingRate(DxShadingRate, Combiners);
}

void CD3D12RHICommandContext::SetShadingRateImage(CRHITexture2D* ShadingImage)
{
    FlushResourceBarriers();

    if (ShadingImage)
    {
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(ShadingImage);
        CmdList.RSSetShadingRateImage(DxTexture->GetResource()->GetResource());

        CmdBatch->AddInUseResource(ShadingImage);
    }
    else
    {
        CmdList.RSSetShadingRateImage(nullptr);
    }
}

void CD3D12RHICommandContext::BeginRenderPass()
{
    // Empty for now
}

void CD3D12RHICommandContext::EndRenderPass()
{
    // Empty for now
}

void CD3D12RHICommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
{
    D3D12_VIEWPORT Viewport;
    Viewport.Width = Width;
    Viewport.Height = Height;
    Viewport.MaxDepth = MaxDepth;
    Viewport.MinDepth = MinDepth;
    Viewport.TopLeftX = x;
    Viewport.TopLeftY = y;

    CmdList.RSSetViewports(&Viewport, 1);
}

void CD3D12RHICommandContext::SetScissorRect(float Width, float Height, float x, float y)
{
    D3D12_RECT ScissorRect;
    ScissorRect.top = LONG(y);
    ScissorRect.bottom = LONG(Height);
    ScissorRect.left = LONG(x);
    ScissorRect.right = LONG(Width);

    CmdList.RSSetScissorRects(&ScissorRect, 1);
}

void CD3D12RHICommandContext::SetBlendFactor(const SColorF& Color)
{
    CmdList.OMSetBlendFactor(Color.Elements);
}

void CD3D12RHICommandContext::SetPrimitiveTopology(EPrimitiveTopology InPrimitveTopology)
{
    const D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = ConvertPrimitiveTopology(InPrimitveTopology);
    if (CurrentPrimitiveTolpology != PrimitiveTopology)
    {
        CmdList.IASetPrimitiveTopology(PrimitiveTopology);
        CurrentPrimitiveTolpology = PrimitiveTopology;
    }
}

void CD3D12RHICommandContext::SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot)
{
    D3D12_ERROR(BufferSlot + BufferCount < D3D12_MAX_VERTEX_BUFFER_SLOTS, "Trying to set a VertexBuffer to an invalid slot");

    for (uint32 i = 0; i < BufferCount; i++)
    {
        uint32 Slot = BufferSlot + i;
        CD3D12RHIVertexBuffer* DxVertexBuffer = static_cast<CD3D12RHIVertexBuffer*>(VertexBuffers[i]);
        DescriptorCache.SetVertexBuffer(DxVertexBuffer, Slot);

        // TODO: The DescriptorCache maybe should have this responsibility?
        CmdBatch->AddInUseResource(DxVertexBuffer);
    }
}

void CD3D12RHICommandContext::SetIndexBuffer(CRHIIndexBuffer* IndexBuffer)
{
    CD3D12RHIIndexBuffer* DxIndexBuffer = static_cast<CD3D12RHIIndexBuffer*>(IndexBuffer);
    DescriptorCache.SetIndexBuffer(DxIndexBuffer);

    // TODO: Maybe this should be done by the descriptor cache
    CmdBatch->AddInUseResource(DxIndexBuffer);
}

void CD3D12RHICommandContext::SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView)
{
    for (uint32 Slot = 0; Slot < RenderTargetCount; Slot++)
    {
        CD3D12RHIRenderTargetView* DxRenderTargetView = static_cast<CD3D12RHIRenderTargetView*>(RenderTargetViews[Slot]);
        DescriptorCache.SetRenderTargetView(DxRenderTargetView, Slot);

        // TODO: Maybe this should be handled by the descriptor cache
        CmdBatch->AddInUseResource(DxRenderTargetView);
    }

    CD3D12RHIDepthStencilView* DxDepthStencilView = static_cast<CD3D12RHIDepthStencilView*>(DepthStencilView);
    DescriptorCache.SetDepthStencilView(DxDepthStencilView);

    CmdBatch->AddInUseResource(DxDepthStencilView);
}

void CD3D12RHICommandContext::SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState)
{
    // TODO: Maybe it should be supported to unbind pipelines by setting it to nullptr
    D3D12_ERROR(PipelineState != nullptr, "PipelineState cannot be nullptr ");

    CD3D12RHIGraphicsPipelineState* DxPipelineState = static_cast<CD3D12RHIGraphicsPipelineState*>(PipelineState);
    if (DxPipelineState != CurrentGraphicsPipelineState)
    {
        CurrentGraphicsPipelineState = MakeSharedRef<CD3D12RHIGraphicsPipelineState>(DxPipelineState);
        CmdList.SetPipelineState(CurrentGraphicsPipelineState->GetPipeline());
    }

    CD3D12RootSignature* DxRootSignature = DxPipelineState->GetRootSignature();
    if (DxRootSignature != CurrentRootSignature)
    {
        CurrentRootSignature = MakeSharedRef<CD3D12RootSignature>(DxRootSignature);
        CmdList.SetGraphicsRootSignature(CurrentRootSignature.Get());
    }
}

void CD3D12RHICommandContext::SetComputePipelineState(class CRHIComputePipelineState* PipelineState)
{
    // TODO: Maybe it should be supported to unbind pipelines by setting it to nullptr
    D3D12_ERROR(PipelineState != nullptr, "PipelineState cannot be nullptr ");

    CD3D12RHIComputePipelineState* DxPipelineState = static_cast<CD3D12RHIComputePipelineState*>(PipelineState);
    if (DxPipelineState != CurrentComputePipelineState.Get())
    {
        CurrentComputePipelineState = MakeSharedRef<CD3D12RHIComputePipelineState>(DxPipelineState);
        CmdList.SetPipelineState(CurrentComputePipelineState->GetPipeline());
    }

    CD3D12RootSignature* DxRootSignature = DxPipelineState->GetRootSignature();
    if (DxRootSignature != CurrentRootSignature.Get())
    {
        CurrentRootSignature = MakeSharedRef<CD3D12RootSignature>(DxRootSignature);
        CmdList.SetComputeRootSignature(CurrentRootSignature.Get());
    }
}

void CD3D12RHICommandContext::Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    UNREFERENCED_VARIABLE(Shader);
    ShaderConstantsCache.Set32BitShaderConstants(reinterpret_cast<const uint32*>(Shader32BitConstants), Num32BitConstants);
}

void CD3D12RHICommandContext::SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetShaderResourceParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    CD3D12RHIShaderResourceView* DxShaderResourceView = static_cast<CD3D12RHIShaderResourceView*>(ShaderResourceView);
    DescriptorCache.SetShaderResourceView(DxShaderResourceView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
}

void CD3D12RHICommandContext::SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetShaderResourceParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumShaderResourceViews, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumShaderResourceViews; i++)
    {
        CD3D12RHIShaderResourceView* DxShaderResourceView = static_cast<CD3D12RHIShaderResourceView*>(ShaderResourceView[i]);
        DescriptorCache.SetShaderResourceView(DxShaderResourceView, DxShader->GetShaderVisibility(), ParameterInfo.Register + i);
    }
}

void CD3D12RHICommandContext::SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetUnorderedAccessParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    CD3D12RHIUnorderedAccessView* DxUnorderedAccessView = static_cast<CD3D12RHIUnorderedAccessView*>(UnorderedAccessView);
    DescriptorCache.SetUnorderedAccessView(DxUnorderedAccessView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
}

void CD3D12RHICommandContext::SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetUnorderedAccessParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumUnorderedAccessViews, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
    {
        CD3D12RHIUnorderedAccessView* DxUnorderedAccessView = static_cast<CD3D12RHIUnorderedAccessView*>(UnorderedAccessViews[i]);
        DescriptorCache.SetUnorderedAccessView(DxUnorderedAccessView, DxShader->GetShaderVisibility(), ParameterInfo.Register + i);
    }
}

void CD3D12RHICommandContext::SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetConstantBufferParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    if (ConstantBuffer)
    {
        CD3D12RHIConstantBufferView& DxConstantBufferView = static_cast<CD3D12RHIConstantBuffer*>(ConstantBuffer)->GetView();
        DescriptorCache.SetConstantBufferView(&DxConstantBufferView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
    }
    else
    {
        DescriptorCache.SetConstantBufferView(nullptr, DxShader->GetShaderVisibility(), ParameterInfo.Register);
    }
}

void CD3D12RHICommandContext::SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetConstantBufferParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumConstantBuffers, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumConstantBuffers; i++)
    {
        if (ConstantBuffers[i])
        {
            CD3D12RHIConstantBufferView& DxConstantBufferView = static_cast<CD3D12RHIConstantBuffer*>(ConstantBuffers[i])->GetView();
            DescriptorCache.SetConstantBufferView(&DxConstantBufferView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
        }
        else
        {
            DescriptorCache.SetConstantBufferView(nullptr, DxShader->GetShaderVisibility(), ParameterInfo.Register);
        }
    }
}

void CD3D12RHICommandContext::SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetSamplerStateParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    CD3D12RHISamplerState* DxSamplerState = static_cast<CD3D12RHISamplerState*>(SamplerState);
    DescriptorCache.SetSamplerState(DxSamplerState, DxShader->GetShaderVisibility(), ParameterInfo.Register);
}

void CD3D12RHICommandContext::SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
{
    CD3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    D3D12_ERROR(DxShader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = DxShader->GetSamplerStateParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumSamplerStates, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumSamplerStates; i++)
    {
        CD3D12RHISamplerState* DxSamplerState = static_cast<CD3D12RHISamplerState*>(SamplerStates[i]);
        DescriptorCache.SetSamplerState(DxSamplerState, DxShader->GetShaderVisibility(), ParameterInfo.Register);
    }
}

void CD3D12RHICommandContext::ResolveTexture(CRHITexture* Destination, CRHITexture* Source)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    CD3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
    CD3D12BaseTexture* DxSource = D3D12TextureCast(Source);
    const DXGI_FORMAT DstFormat = DxDestination->GetNativeFormat();
    const DXGI_FORMAT SrcFormat = DxSource->GetNativeFormat();

    //TODO: For now texture must be the same format. I.e typeless does probably not work
    D3D12_ERROR(DstFormat == SrcFormat, "Destination and Source texture must have the same format");

    CmdList.ResolveSubresource(DxDestination->GetResource(), DxSource->GetResource(), DstFormat);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12RHICommandContext::UpdateBuffer(CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    if (SizeInBytes > 0)
    {
        CD3D12BaseBuffer* DxDestination = D3D12BufferCast(Destination);
        UpdateBuffer(DxDestination->GetResource(), OffsetInBytes, SizeInBytes, SourceData);

        CmdBatch->AddInUseResource(Destination);
    }
}

void CD3D12RHICommandContext::UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
    D3D12_ERROR(Destination != nullptr, "Destination cannot be nullptr");

    if (Width > 0 && Height > 0)
    {
        D3D12_ERROR(SourceData != nullptr, "SourceData cannot be nullptr");

        FlushResourceBarriers();

        CD3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
        const DXGI_FORMAT NativeFormat = DxDestination->GetNativeFormat();
        const uint32 Stride = GetFormatStride(NativeFormat);
        const uint32 RowPitch = ((Width * Stride) + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
        const uint32 SizeInBytes = Height * RowPitch;

        CD3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();
        SD3D12UploadAllocation Allocation = GpuResourceUploader.LinearAllocate(SizeInBytes);

        const uint8* Source = reinterpret_cast<const uint8*>(SourceData);
        for (uint32 y = 0; y < Height; y++)
        {
            CMemory::Memcpy(Allocation.MappedPtr + (y * RowPitch), Source + (y * Width * Stride), Width * Stride);
        }

        // Copy to Dest
        D3D12_TEXTURE_COPY_LOCATION SourceLocation;
        CMemory::Memzero(&SourceLocation);

        SourceLocation.pResource = GpuResourceUploader.GetGpuResource();
        SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        SourceLocation.PlacedFootprint.Footprint.Format = NativeFormat;
        SourceLocation.PlacedFootprint.Footprint.Width = Width;
        SourceLocation.PlacedFootprint.Footprint.Height = Height;
        SourceLocation.PlacedFootprint.Footprint.Depth = 1;
        SourceLocation.PlacedFootprint.Footprint.RowPitch = RowPitch;
        SourceLocation.PlacedFootprint.Offset = Allocation.ResourceOffset;

        // TODO: Miplevel may not be the correct subresource
        D3D12_TEXTURE_COPY_LOCATION DestLocation;
        CMemory::Memzero(&DestLocation);

        DestLocation.pResource = DxDestination->GetResource()->GetResource();
        DestLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DestLocation.SubresourceIndex = MipLevel;

        CmdList.CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

        CmdBatch->AddInUseResource(Destination);
    }
}

void CD3D12RHICommandContext::CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SCopyBufferInfo& CopyInfo)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    CD3D12BaseBuffer* DxDestination = D3D12BufferCast(Destination);
    CD3D12BaseBuffer* DxSource = D3D12BufferCast(Source);
    CmdList.CopyBufferRegion(DxDestination->GetResource(), CopyInfo.DestinationOffset, DxSource->GetResource(), CopyInfo.SourceOffset, CopyInfo.SizeInBytes);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12RHICommandContext::CopyTexture(CRHITexture* Destination, CRHITexture* Source)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    CD3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
    CD3D12BaseTexture* DxSource = D3D12TextureCast(Source);
    CmdList.CopyResource(DxDestination->GetResource(), DxSource->GetResource());

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12RHICommandContext::CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SCopyTextureInfo& CopyInfo)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    CD3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
    CD3D12BaseTexture* DxSource = D3D12TextureCast(Source);

    // Source
    D3D12_TEXTURE_COPY_LOCATION SourceLocation;
    CMemory::Memzero(&SourceLocation);

    SourceLocation.pResource = DxSource->GetResource()->GetResource();
    SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    SourceLocation.SubresourceIndex = CopyInfo.Source.SubresourceIndex;

    D3D12_BOX SourceBox;
    SourceBox.left = CopyInfo.Source.x;
    SourceBox.right = CopyInfo.Source.x + CopyInfo.Width;
    SourceBox.bottom = CopyInfo.Source.y;
    SourceBox.top = CopyInfo.Source.y + CopyInfo.Height;
    SourceBox.front = CopyInfo.Source.z;
    SourceBox.back = CopyInfo.Source.z + CopyInfo.Depth;

    // Destination
    D3D12_TEXTURE_COPY_LOCATION DestinationLocation;
    CMemory::Memzero(&DestinationLocation);

    DestinationLocation.pResource = DxDestination->GetResource()->GetResource();
    DestinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestinationLocation.SubresourceIndex = CopyInfo.Destination.SubresourceIndex;

    FlushResourceBarriers();

    CmdList.CopyTextureRegion(&DestinationLocation, CopyInfo.Destination.x, CopyInfo.Destination.y, CopyInfo.Destination.z, &SourceLocation, &SourceBox);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12RHICommandContext::DestroyResource(CRHIResource* Resource)
{
    CmdBatch->AddInUseResource(Resource);
}

void CD3D12RHICommandContext::DiscardResource(CRHIMemoryResource* Resource)
{
    // TODO: Enable regions to be discarded

    CD3D12Resource* DxResource = D3D12ResourceCast(Resource);
    if (DxResource)
    {
        CmdList.DiscardResource(DxResource->GetResource(), nullptr);
        CmdBatch->AddInUseResource(Resource);
    }
}

void CD3D12RHICommandContext::BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate)
{
    D3D12_ERROR(Geometry != nullptr, "Geometry cannot be nullptr");

    FlushResourceBarriers();

    CD3D12RHIVertexBuffer* DxVertexBuffer = static_cast<CD3D12RHIVertexBuffer*>(VertexBuffer);
    CD3D12RHIIndexBuffer* DxIndexBuffer = static_cast<CD3D12RHIIndexBuffer*>(IndexBuffer);
    D3D12_ERROR(DxVertexBuffer != nullptr, "VertexBuffer cannot be nullptr");

    CD3D12RHIRayTracingGeometry* DxGeometry = static_cast<CD3D12RHIRayTracingGeometry*>(Geometry);
    DxGeometry->VertexBuffer = DxVertexBuffer;
    DxGeometry->IndexBuffer = DxIndexBuffer;
    DxGeometry->Build(*this, bUpdate);

    CmdBatch->AddInUseResource(Geometry);
    CmdBatch->AddInUseResource(VertexBuffer);
    CmdBatch->AddInUseResource(IndexBuffer);
}

void CD3D12RHICommandContext::BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate)
{
    D3D12_ERROR(RayTracingScene != nullptr, "RayTracingScene cannot be nullptr");

    FlushResourceBarriers();

    CD3D12RHIRayTracingScene* DxScene = static_cast<CD3D12RHIRayTracingScene*>(RayTracingScene);
    DxScene->Build(*this, Instances, NumInstances, bUpdate);

    CmdBatch->AddInUseResource(RayTracingScene);
}

void CD3D12RHICommandContext::SetRayTracingBindings(
    CRHIRayTracingScene* RayTracingScene,
    CRHIRayTracingPipelineState* PipelineState,
    const SRayTracingShaderResources* GlobalResource,
    const SRayTracingShaderResources* RayGenLocalResources,
    const SRayTracingShaderResources* MissLocalResources,
    const SRayTracingShaderResources* HitGroupResources,
    uint32 NumHitGroupResources)
{
    CD3D12RHIRayTracingScene* DxScene = static_cast<CD3D12RHIRayTracingScene*>(RayTracingScene);
    CD3D12RHIRayTracingPipelineState* DxPipelineState = static_cast<CD3D12RHIRayTracingPipelineState*>(PipelineState);
    D3D12_ERROR(DxScene != nullptr, "RayTracingScene cannot be nullptr");
    D3D12_ERROR(DxPipelineState != nullptr, "PipelineState cannot be nullptr");

    uint32 NumDescriptorsNeeded = 0;
    uint32 NumSamplersNeeded = 0;
    if (GlobalResource)
    {
        NumDescriptorsNeeded += GlobalResource->NumResources();
        NumSamplersNeeded += GlobalResource->NumSamplers();
    }
    if (RayGenLocalResources)
    {
        NumDescriptorsNeeded += RayGenLocalResources->NumResources();
        NumSamplersNeeded += RayGenLocalResources->NumSamplers();
    }
    if (MissLocalResources)
    {
        NumDescriptorsNeeded += MissLocalResources->NumResources();
        NumSamplersNeeded += MissLocalResources->NumSamplers();
    }

    for (uint32 i = 0; i < NumHitGroupResources; i++)
    {
        NumDescriptorsNeeded += HitGroupResources[i].NumResources();
        NumSamplersNeeded += HitGroupResources[i].NumSamplers();
    }

    D3D12_ERROR(NumDescriptorsNeeded < D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, "NumDescriptorsNeeded=" + ToString(NumDescriptorsNeeded) + ", but the maximum is '" + ToString(D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT) + "'");

    CD3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    if (!ResourceHeap->HasSpace(NumDescriptorsNeeded))
    {
        ResourceHeap->AllocateFreshHeap();
    }

    D3D12_ERROR(NumSamplersNeeded < D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, "NumDescriptorsNeeded=" + ToString(NumSamplersNeeded) + ", but the maximum is '" + ToString(D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT) + "'");

    CD3D12OnlineDescriptorHeap* SamplerHeap = CmdBatch->GetOnlineSamplerDescriptorHeap();
    if (!SamplerHeap->HasSpace(NumSamplersNeeded))
    {
        SamplerHeap->AllocateFreshHeap();
    }

    if (!DxScene->BuildBindingTable(*this, DxPipelineState, ResourceHeap, SamplerHeap, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources))
    {
        D3D12_ERROR_ALWAYS("[D3D12CommandContext]: FAILED to Build Shader Binding Table");
    }

    if (GlobalResource)
    {
        if (!GlobalResource->ConstantBuffers.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ConstantBuffers.Size(); i++)
            {
                CD3D12RHIConstantBufferView& DxConstantBufferView = static_cast<CD3D12RHIConstantBuffer*>(GlobalResource->ConstantBuffers[i])->GetView();
                DescriptorCache.SetConstantBufferView(&DxConstantBufferView, ShaderVisibility_All, i);
            }
        }
        if (!GlobalResource->ShaderResourceViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ShaderResourceViews.Size(); i++)
            {
                CD3D12RHIShaderResourceView* DxShaderResourceView = static_cast<CD3D12RHIShaderResourceView*>(GlobalResource->ShaderResourceViews[i]);
                DescriptorCache.SetShaderResourceView(DxShaderResourceView, ShaderVisibility_All, i);
            }
        }
        if (!GlobalResource->UnorderedAccessViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->UnorderedAccessViews.Size(); i++)
            {
                CD3D12RHIUnorderedAccessView* DxUnorderedAccessView = static_cast<CD3D12RHIUnorderedAccessView*>(GlobalResource->UnorderedAccessViews[i]);
                DescriptorCache.SetUnorderedAccessView(DxUnorderedAccessView, ShaderVisibility_All, i);
            }
        }
        if (!GlobalResource->SamplerStates.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->SamplerStates.Size(); i++)
            {
                CD3D12RHISamplerState* DxSampler = static_cast<CD3D12RHISamplerState*>(GlobalResource->SamplerStates[i]);
                DescriptorCache.SetSamplerState(DxSampler, ShaderVisibility_All, i);
            }
        }
    }

    ID3D12GraphicsCommandList4* DXRCommandList = CmdList.GetDXRCommandList();

    CD3D12RootSignature* GlobalRootSignature = DxPipelineState->GetGlobalRootSignature();
    CurrentRootSignature = MakeSharedRef<CD3D12RootSignature>(GlobalRootSignature);

    DXRCommandList->SetComputeRootSignature(CurrentRootSignature->GetRootSignature());

    DescriptorCache.CommitComputeDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());
}

void CD3D12RHICommandContext::GenerateMips(CRHITexture* Texture)
{
    CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
    D3D12_ERROR(DxTexture != nullptr, "Texture cannot be nullptr");

    D3D12_RESOURCE_DESC Desc = DxTexture->GetResource()->GetDesc();
    Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_ERROR(Desc.MipLevels > 1, "MipLevels must be more than one in order to generate any MipLevels");

    // TODO: Create this placed from a Heap? See what performance is 
    TSharedRef<CD3D12Resource> StagingTexture = dbg_new CD3D12Resource(GetDevice(), Desc, DxTexture->GetResource()->GetHeapType());
    if (!StagingTexture->Init(D3D12_RESOURCE_STATE_COMMON, nullptr))
    {
        LOG_ERROR("[D3D12CommandContext] Failed to create StagingTexture for GenerateMips");
        return;
    }
    else
    {
        StagingTexture->SetName("GenerateMips StagingTexture");
    }

    // Check Type
    const bool bIsTextureCube = (Texture->AsTextureCube() != nullptr);

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
    CMemory::Memzero(&SrvDesc);

    SrvDesc.Format = Desc.Format;
    SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (bIsTextureCube)
    {
        SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        SrvDesc.TextureCube.MipLevels = Desc.MipLevels;
        SrvDesc.TextureCube.MostDetailedMip = 0;
        SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SrvDesc.Texture2D.MipLevels = Desc.MipLevels;
        SrvDesc.Texture2D.MostDetailedMip = 0;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
    CMemory::Memzero(&UavDesc);

    UavDesc.Format = Desc.Format;
    if (bIsTextureCube)
    {
        UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UavDesc.Texture2DArray.ArraySize = 6;
        UavDesc.Texture2DArray.FirstArraySlice = 0;
        UavDesc.Texture2DArray.PlaneSlice = 0;
    }
    else
    {
        UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        UavDesc.Texture2D.PlaneSlice = 0;
    }

    const uint32 MipLevelsPerDispatch = 4;
    const uint32 UavDescriptorHandleCount = NMath::AlignUp<uint32>(Desc.MipLevels, MipLevelsPerDispatch);
    const uint32 NumDispatches = UavDescriptorHandleCount / MipLevelsPerDispatch;

    CD3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();

    // Allocate an extra handle for SRV
    const uint32 StartDescriptorHandleIndex = ResourceHeap->AllocateHandles(UavDescriptorHandleCount + 1);

    const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(StartDescriptorHandleIndex);
    GetDevice()->CreateShaderResourceView(DxTexture->GetResource()->GetResource(), &SrvDesc, SrvHandle_CPU);

    const uint32 UavStartDescriptorHandleIndex = StartDescriptorHandleIndex + 1;
    for (uint32 i = 0; i < Desc.MipLevels; i++)
    {
        if (bIsTextureCube)
        {
            UavDesc.Texture2DArray.MipSlice = i;
        }
        else
        {
            UavDesc.Texture2D.MipSlice = i;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE UavHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(UavStartDescriptorHandleIndex + i);
        GetDevice()->CreateUnorderedAccessView(StagingTexture->GetResource(), nullptr, &UavDesc, UavHandle_CPU);
    }

    for (uint32 i = Desc.MipLevels; i < UavDescriptorHandleCount; i++)
    {
        if (bIsTextureCube)
        {
            UavDesc.Texture2DArray.MipSlice = 0;
        }
        else
        {
            UavDesc.Texture2D.MipSlice = 0;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE UavHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(UavStartDescriptorHandleIndex + i);
        GetDevice()->CreateUnorderedAccessView(nullptr, nullptr, &UavDesc, UavHandle_CPU);
    }

    // We assume the destination is in D3D12_RESOURCE_STATE_COPY_DEST
    TransitionResource(DxTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushResourceBarriers();

    CmdList.CopyResource(StagingTexture.Get(), DxTexture->GetResource());

    TransitionResource(DxTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    FlushResourceBarriers();

    if (bIsTextureCube)
    {
        TSharedRef<CD3D12RHIComputePipelineState> PipelineState = GD3D12RHICore->GetGenerateMipsPipelineTexureCube();
        CmdList.SetPipelineState(PipelineState->GetPipeline());
        CmdList.SetComputeRootSignature(PipelineState->GetRootSignature());
    }
    else
    {
        TSharedRef<CD3D12RHIComputePipelineState> PipelineState = GD3D12RHICore->GetGenerateMipsPipelineTexure2D();
        CmdList.SetPipelineState(PipelineState->GetPipeline());
        CmdList.SetComputeRootSignature(PipelineState->GetRootSignature());
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE SrvHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(StartDescriptorHandleIndex);

    ID3D12DescriptorHeap* OnlineResourceHeap = ResourceHeap->GetHeap()->GetHeap();
    CmdList.SetDescriptorHeaps(&OnlineResourceHeap, 1);

    struct ConstantBuffer
    {
        uint32   SrcMipLevel;
        uint32   NumMipLevels;
        CVector2 TexelSize;
    } ConstantData;

    uint32 DstWidth = static_cast<uint32>(Desc.Width);
    uint32 DstHeight = Desc.Height;
    ConstantData.SrcMipLevel = 0;

    const uint32 ThreadsZ = bIsTextureCube ? 6 : 1;

    uint32 RemainingMiplevels = Desc.MipLevels;
    for (uint32 i = 0; i < NumDispatches; i++)
    {
        ConstantData.TexelSize = CVector2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));
        ConstantData.NumMipLevels = NMath::Min<uint32>(4, RemainingMiplevels);

        CmdList.SetComputeRoot32BitConstants(&ConstantData, 4, 0, 0);
        CmdList.SetComputeRootDescriptorTable(SrvHandle_GPU, 1);

        const uint32 GPUDescriptorHandleIndex = i * MipLevelsPerDispatch;

        const D3D12_GPU_DESCRIPTOR_HANDLE UavHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(UavStartDescriptorHandleIndex + GPUDescriptorHandleIndex);
        CmdList.SetComputeRootDescriptorTable(UavHandle_GPU, 2);

        constexpr uint32 ThreadCount = 8;

        const uint32 ThreadsX = NMath::DivideByMultiple(DstWidth, ThreadCount);
        const uint32 ThreadsY = NMath::DivideByMultiple(DstHeight, ThreadCount);
        CmdList.Dispatch(ThreadsX, ThreadsY, ThreadsZ);

        UnorderedAccessBarrier(StagingTexture.Get());

        TransitionResource(DxTexture->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        // TODO: Copy only miplevels (Maybe faster?)
        CmdList.CopyResource(DxTexture->GetResource(), StagingTexture.Get());

        TransitionResource(DxTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        FlushResourceBarriers();

        DstWidth = DstWidth / 16;
        DstHeight = DstHeight / 16;

        ConstantData.SrcMipLevel += 3;
        RemainingMiplevels -= MipLevelsPerDispatch;
    }

    TransitionResource(DxTexture->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushResourceBarriers();

    CmdBatch->AddInUseResource(Texture);
    CmdBatch->AddInUseResource(StagingTexture.Get());
}

void CD3D12RHICommandContext::TransitionTexture(CRHITexture* Texture, EResourceState BeforeState, EResourceState AfterState)
{
    const D3D12_RESOURCE_STATES DxBeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES DxAfterState = ConvertResourceState(AfterState);

    CD3D12BaseTexture* Resource = D3D12TextureCast(Texture);
    TransitionResource(Resource->GetResource(), DxBeforeState, DxAfterState);

    CmdBatch->AddInUseResource(Texture);
}

void CD3D12RHICommandContext::TransitionBuffer(CRHIBuffer* Buffer, EResourceState BeforeState, EResourceState AfterState)
{
    const D3D12_RESOURCE_STATES DxBeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES DxAfterState = ConvertResourceState(AfterState);

    CD3D12BaseBuffer* Resource = D3D12BufferCast(Buffer);
    TransitionResource(Resource->GetResource(), DxBeforeState, DxAfterState);

    CmdBatch->AddInUseResource(Buffer);
}

void CD3D12RHICommandContext::UnorderedAccessTextureBarrier(CRHITexture* Texture)
{
    CD3D12BaseTexture* Resource = D3D12TextureCast(Texture);
    UnorderedAccessBarrier(Resource->GetResource());

    CmdBatch->AddInUseResource(Texture);
}

void CD3D12RHICommandContext::UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)
{
    CD3D12BaseBuffer* Resource = D3D12BufferCast(Buffer);
    UnorderedAccessBarrier(Resource->GetResource());

    CmdBatch->AddInUseResource(Buffer);
}

void CD3D12RHICommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    FlushResourceBarriers();

    if (VertexCount)
    {
        ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

        CmdList.DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
    }
}

void CD3D12RHICommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    FlushResourceBarriers();

    if (IndexCount)
    {
        ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

        CmdList.DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
    }
}

void CD3D12RHICommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    if (VertexCountPerInstance > 0 && InstanceCount > 0)
    {
        ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

        CmdList.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }
}

void CD3D12RHICommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    if (IndexCountPerInstance > 0 && InstanceCount > 0)
    {
        ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

        CmdList.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }
}

void CD3D12RHICommandContext::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    FlushResourceBarriers();

    if (ThreadGroupCountX > 0 || ThreadGroupCountY > 0 || ThreadGroupCountZ > 0)
    {
        ShaderConstantsCache.CommitCompute(CmdList, CurrentRootSignature.Get());
        DescriptorCache.CommitComputeDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

        CmdList.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }
}

void CD3D12RHICommandContext::DispatchRays(CRHIRayTracingScene* RayTracingScene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    CD3D12RHIRayTracingScene* DxScene = static_cast<CD3D12RHIRayTracingScene*>(RayTracingScene);
    D3D12_ERROR(DxScene != nullptr, "RayTracingScene cannot be nullptr");

    CD3D12RHIRayTracingPipelineState* DxPipelineState = static_cast<CD3D12RHIRayTracingPipelineState*>(PipelineState);
    D3D12_ERROR(DxPipelineState != nullptr, "PipelineState cannot be nullptr");

    ID3D12GraphicsCommandList4* DXRCommandList = CmdList.GetDXRCommandList();
    D3D12_ERROR(DXRCommandList != nullptr, "DXRCommandList is nullptr, DXR is not supported");

    FlushResourceBarriers();

    if (Width > 0 || Height > 0 || Depth > 0)
    {
        D3D12_DISPATCH_RAYS_DESC RayDispatchDesc;
        CMemory::Memzero(&RayDispatchDesc);

        RayDispatchDesc.RayGenerationShaderRecord = DxScene->GetRayGenShaderRecord();
        RayDispatchDesc.MissShaderTable = DxScene->GetMissShaderTable();
        RayDispatchDesc.HitGroupTable = DxScene->GetHitGroupTable();

        RayDispatchDesc.Width = Width;
        RayDispatchDesc.Height = Height;
        RayDispatchDesc.Depth = Depth;

        DXRCommandList->SetPipelineState1(DxPipelineState->GetStateObject());
        DXRCommandList->DispatchRays(&RayDispatchDesc);
    }
}

void CD3D12RHICommandContext::ClearState()
{
    Flush();

    InternalClearState();
}

void CD3D12RHICommandContext::Flush()
{
    const uint64 NewFenceValue = ++FenceValue;
    if (!CmdQueue.SignalFence(Fence, NewFenceValue))
    {
        return;
    }

    Fence.WaitForValue(FenceValue);

    for (CD3D12CommandBatch& Batch : CmdBatches)
    {
        Batch.Reset();
    }
}

void CD3D12RHICommandContext::InsertMarker(const CString& Message)
{
    if (ND3D12Functions::SetMarkerOnCommandList)
    {
        ND3D12Functions::SetMarkerOnCommandList(CmdList.GetGraphicsCommandList(), PIX_COLOR(255, 255, 255), Message.CStr());
    }
}

void CD3D12RHICommandContext::BeginExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetGraphicsAnalysisInterface();
    if (GraphicsAnalysis && !bIsCapturing)
    {
        GraphicsAnalysis->BeginCapture();
        bIsCapturing = true;
    }
}

void CD3D12RHICommandContext::EndExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetGraphicsAnalysisInterface();
    if (GraphicsAnalysis && bIsCapturing)
    {
        GraphicsAnalysis->EndCapture();
        bIsCapturing = false;
    }
}

void CD3D12RHICommandContext::InternalClearState()
{
    DescriptorCache.Reset();
    ShaderConstantsCache.Reset();

    CurrentGraphicsPipelineState.Reset();
    CurrentRootSignature.Reset();
    CurrentComputePipelineState.Reset();

    CurrentPrimitiveTolpology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
