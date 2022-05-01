#include "D3D12Device.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandList.h"
#include "D3D12Core.h"
#include "D3D12Shader.h"
#include "D3D12CoreInstance.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12PipelineState.h"
#include "D3D12RayTracing.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12RHITimestampQuery.h"
#include "D3D12CommandContext.h"
#include "D3D12FunctionPointers.h"

#include "Core/Math/Vector2.h"
#include "Core/Debug/Profiler/FrameProfiler.h"

#include <pix.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ResourceBarrierBatcher

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

        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = Resource;
        Barrier.Transition.StateAfter  = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        Barriers.Emplace(Barrier);
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12GPUResourceUploader

CD3D12GPUResourceUploader::CD3D12GPUResourceUploader(CD3D12Device* InDevice)
    : CD3D12DeviceChild(InDevice)
    , MappedMemory(nullptr)
    , SizeInBytes(0)
    , OffsetInBytes(0)
    , Resource(nullptr)
    , GarbageResources()
{ }

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

    HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Width              = InSizeInBytes;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels          = 1;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    HRESULT Result = GetDevice()->CreateCommitedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&Resource));
    if (SUCCEEDED(Result))
    {
        Resource->SetName(L"[D3D12GPUResourceUploader] Buffer");
        Resource->Map(0, nullptr, reinterpret_cast<void**>(&MappedMemory));

        SizeInBytes   = InSizeInBytes;
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
    Allocation.MappedPtr      = MappedMemory + OffsetInBytes;
    Allocation.ResourceOffset = OffsetInBytes;
    OffsetInBytes            += InSizeInBytes;
    return Allocation;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12CommandBatch

CD3D12CommandBatch::CD3D12CommandBatch(CD3D12Device* InDevice)
    : Device(InDevice)
    , CmdAllocator(InDevice)
    , GpuResourceUploader(InDevice)
    , OnlineResourceDescriptorHeap(nullptr)
    , OnlineSamplerDescriptorHeap(nullptr)
    , Resources()
{ }

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
// CD3D12RHICommandContext

CD3D12CommandContext* CD3D12CommandContext::Make(CD3D12Device* InDevice)
{
    CD3D12CommandContext* NewContext = dbg_new CD3D12CommandContext(InDevice);
    if (NewContext && NewContext->Initialize())
    {
        return NewContext;
    }
    else
    {
        SafeDelete(NewContext);
        return nullptr;
    }
}

CD3D12CommandContext::CD3D12CommandContext(CD3D12Device* InDevice)
    : IRHICommandContext()
    , CD3D12DeviceChild(InDevice)
    , CommandQueue(InDevice)
    , CommandList(InDevice)
    , Fence(InDevice)
    , DescriptorCache(InDevice)
    , CmdBatches()
    , BarrierBatcher()
{ }

CD3D12CommandContext::~CD3D12CommandContext()
{
    Flush();
}

bool CD3D12CommandContext::Initialize()
{
    if (!CommandQueue.Init(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    for (uint32 Index = 0; Index < D3D12_NUM_BACK_BUFFERS; ++Index)
    {
        CD3D12CommandBatch& Batch = CmdBatches.Emplace(GetDevice());
        if (!Batch.Init())
        {
            D3D12_ERROR_ALWAYS("Failed to initialize D3D12CommandBatch");
            return false;
        }
    }

    if (!CommandList.Init(D3D12_COMMAND_LIST_TYPE_DIRECT, CmdBatches[0].GetCommandAllocator(), nullptr))
    {
        D3D12_ERROR_ALWAYS("Failed to initialize CommandList");
        return false;
    }

    FenceValue = 0;
    if (!Fence.Initialize(FenceValue))
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

void CD3D12CommandContext::UpdateBuffer(CD3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    D3D12_ERROR(Resource != nullptr, "Resource cannot be nullptr");

    if (SizeInBytes)
    {
        D3D12_ERROR(SourceData != nullptr, "SourceData cannot be nullptr ");

        FlushResourceBarriers();

        CD3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();

        SD3D12UploadAllocation Allocation = GpuResourceUploader.LinearAllocate((uint32)SizeInBytes);
        CMemory::Memcpy(Allocation.MappedPtr, SourceData, SizeInBytes);

        CommandList.CopyBufferRegion(Resource->GetResource(), OffsetInBytes, GpuResourceUploader.GetGpuResource(), Allocation.ResourceOffset, SizeInBytes);

        CmdBatch->AddInUseResource(Resource);
    }
}

void CD3D12CommandContext::StartContext()
{
    Assert(bIsReady == false);

    TRACE_FUNCTION_SCOPE();

    CmdBatch     = &CmdBatches[NextCmdBatch];
    NextCmdBatch = (NextCmdBatch + 1) % CmdBatches.Size();

    if (CmdBatch->AssignedFenceValue >= Fence.GetCompletedValue())
    {
        Fence.WaitForValue(CmdBatch->AssignedFenceValue);
    }

    if (!CmdBatch->Reset())
    {
        D3D12_ERROR_ALWAYS("Failed to reset D3D12CommandBatch");
        return;
    }

    InternalClearState();

    if (!CommandList.Reset(CmdBatch->GetCommandAllocator()))
    {
        D3D12_ERROR_ALWAYS("Failed to reset Commandlist");
    }

    bIsReady = true;
}

void CD3D12CommandContext::FinishContext()
{
    Assert(bIsReady == true);

    TRACE_FUNCTION_SCOPE();

    FlushResourceBarriers();

    const uint64 NewFenceValue = ++FenceValue;
    CmdBatch->AssignedFenceValue = NewFenceValue;
    CmdBatch = nullptr;

    for (int32 QueryIndex = 0; QueryIndex < ResolveQueries.Size(); ++QueryIndex)
    {
        ResolveQueries[QueryIndex]->ResolveQueries(*this);
    }

    ResolveQueries.Clear();

    // Execute
    if (!CommandList.Close())
    {
        D3D12_ERROR_ALWAYS("Failed to close CommandList");
        return;
    }

    CommandQueue.ExecuteCommandList(&CommandList);

    if (!CommandQueue.SignalFence(Fence, NewFenceValue))
    {
        D3D12_ERROR_ALWAYS("Failed to signal Fence on the GPU");
    }

    bIsReady = false;
}

void CD3D12CommandContext::BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)
{
    CD3D12RHITimestampQuery* D3D12TimestampQuery = static_cast<CD3D12RHITimestampQuery*>(TimestampQuery);
    D3D12_ERROR(D3D12TimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* DxCmdList = CommandList.GetGraphicsCommandList();
    D3D12TimestampQuery->BeginQuery(DxCmdList, Index);

    ResolveQueries.Emplace(MakeSharedRef<CD3D12RHITimestampQuery>(D3D12TimestampQuery));
}

void CD3D12CommandContext::EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)
{
    CD3D12RHITimestampQuery* D3D12TimestampQuery = static_cast<CD3D12RHITimestampQuery*>(TimestampQuery);
    D3D12_ERROR(D3D12TimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* D3D12CmdList = CommandList.GetGraphicsCommandList();
    D3D12TimestampQuery->EndQuery(D3D12CmdList, Index);
}

void CD3D12CommandContext::ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const TStaticArray<float, 4>& ClearColor)
{
    D3D12_ERROR(RenderTargetView != nullptr, "RenderTargetView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    CD3D12RenderTargetView* D3D12RenderTargetView = static_cast<CD3D12RenderTargetView*>(RenderTargetView);
    CmdBatch->AddInUseResource(D3D12RenderTargetView);

    CommandList.ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), ClearColor.Elements, 0, nullptr);
}

void CD3D12CommandContext::ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
    D3D12_ERROR(DepthStencilView != nullptr, "DepthStencilView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    CD3D12DepthStencilView* D3D12DepthStencilView = static_cast<CD3D12DepthStencilView*>(DepthStencilView);
    CmdBatch->AddInUseResource(D3D12DepthStencilView);

    CommandList.ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Depth, Stencil);
}

void CD3D12CommandContext::ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor)
{
    D3D12_ERROR(UnorderedAccessView != nullptr, "UnorderedAccessView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    CD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<CD3D12UnorderedAccessView*>(UnorderedAccessView);
    CmdBatch->AddInUseResource(D3D12UnorderedAccessView);

    CD3D12OnlineDescriptorHeap* OnlineDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    const uint32 OnlineDescriptorHandleIndex = OnlineDescriptorHeap->AllocateHandles(1);

    const D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle    = D3D12UnorderedAccessView->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle_CPU = OnlineDescriptorHeap->GetCPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    GetDevice()->CopyDescriptorsSimple(1, OnlineHandle_CPU, OfflineHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandle_GPU = OnlineDescriptorHeap->GetGPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    CommandList.ClearUnorderedAccessViewFloat(OnlineHandle_GPU, D3D12UnorderedAccessView, ClearColor.Elements);
}

void CD3D12CommandContext::SetShadingRate(EShadingRate ShadingRate)
{
    D3D12_SHADING_RATE D3D12ShadingRate = ConvertShadingRate(ShadingRate);

    D3D12_SHADING_RATE_COMBINER Combiners[] =
    {
        D3D12_SHADING_RATE_COMBINER_OVERRIDE,
        D3D12_SHADING_RATE_COMBINER_OVERRIDE,
    };

    CommandList.RSSetShadingRate(D3D12ShadingRate, Combiners);
}

void CD3D12CommandContext::SetShadingRateImage(CRHITexture2D* ShadingImage)
{
    FlushResourceBarriers();

    if (ShadingImage)
    {
        CD3D12Texture* D3D12Texture = D3D12TextureCast(ShadingImage);
        CommandList.RSSetShadingRateImage(D3D12Texture->GetD3D12Resource()->GetResource());

        CmdBatch->AddInUseResource(ShadingImage);
    }
    else
    {
        CommandList.RSSetShadingRateImage(nullptr);
    }
}

void CD3D12CommandContext::BeginRenderPass()
{
    // Empty for now
}

void CD3D12CommandContext::EndRenderPass()
{
    // Empty for now
}

void CD3D12CommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
{
    D3D12_VIEWPORT Viewport;
    Viewport.Width    = Width;
    Viewport.Height   = Height;
    Viewport.MaxDepth = MaxDepth;
    Viewport.MinDepth = MinDepth;
    Viewport.TopLeftX = x;
    Viewport.TopLeftY = y;

    CommandList.RSSetViewports(&Viewport, 1);
}

void CD3D12CommandContext::SetScissorRect(float Width, float Height, float x, float y)
{
    D3D12_RECT ScissorRect;
    ScissorRect.top    = LONG(y);
    ScissorRect.bottom = LONG(Height);
    ScissorRect.left   = LONG(x);
    ScissorRect.right  = LONG(Width);

    CommandList.RSSetScissorRects(&ScissorRect, 1);
}

void CD3D12CommandContext::SetBlendFactor(const TStaticArray<float, 4>& Color)
{
    CommandList.OMSetBlendFactor(Color.Elements);
}

void CD3D12CommandContext::SetPrimitiveTopology(EPrimitiveTopology InPrimitveTopology)
{
    const D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = ConvertPrimitiveTopology(InPrimitveTopology);
    if (CurrentPrimitiveTolpology != PrimitiveTopology)
    {
        CommandList.IASetPrimitiveTopology(PrimitiveTopology);
        CurrentPrimitiveTolpology = PrimitiveTopology;
    }
}

void CD3D12CommandContext::SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot)
{
    D3D12_ERROR(BufferSlot + BufferCount < D3D12_MAX_VERTEX_BUFFER_SLOTS, "Trying to set a VertexBuffer to an invalid slot");

    for (uint32 Index = 0; Index < BufferCount; ++Index)
    {
        CD3D12VertexBuffer* D3D12VertexBuffer = static_cast<CD3D12VertexBuffer*>(VertexBuffers[Index]);
        DescriptorCache.SetVertexBuffer(D3D12VertexBuffer, BufferSlot + Index);
    }
}

void CD3D12CommandContext::SetIndexBuffer(CRHIIndexBuffer* IndexBuffer)
{
    CD3D12IndexBuffer* D3D12IndexBuffer = static_cast<CD3D12IndexBuffer*>(IndexBuffer);
    DescriptorCache.SetIndexBuffer(D3D12IndexBuffer);
}

void CD3D12CommandContext::SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView)
{
    for (uint32 Slot = 0; Slot < RenderTargetCount; Slot++)
    {
        CD3D12RenderTargetView* D3D12RenderTargetView = static_cast<CD3D12RenderTargetView*>(RenderTargetViews[Slot]);
        DescriptorCache.SetRenderTargetView(D3D12RenderTargetView, Slot);

        // TODO: Maybe this should be handled by the descriptor cache
        CmdBatch->AddInUseResource(D3D12RenderTargetView);
    }

    CD3D12DepthStencilView* D3D12DepthStencilView = static_cast<CD3D12DepthStencilView*>(DepthStencilView);
    DescriptorCache.SetDepthStencilView(D3D12DepthStencilView);

    CmdBatch->AddInUseResource(D3D12DepthStencilView);
}

void CD3D12CommandContext::SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState)
{
    // TODO: Maybe it should be supported to unbind pipelines by setting it to nullptr
    D3D12_ERROR(PipelineState != nullptr, "PipelineState cannot be nullptr ");

    CD3D12GraphicsPipelineState* D3D12PipelineState = static_cast<CD3D12GraphicsPipelineState*>(PipelineState);
    if (D3D12PipelineState != CurrentGraphicsPipelineState)
    {
        CurrentGraphicsPipelineState = MakeSharedRef<CD3D12GraphicsPipelineState>(D3D12PipelineState);
        CommandList.SetPipelineState(CurrentGraphicsPipelineState->GetPipeline());
    }

    CD3D12RootSignature* D3D12RootSignature = D3D12PipelineState->GetRootSignature();
    if (D3D12RootSignature != CurrentRootSignature)
    {
        CurrentRootSignature = MakeSharedRef<CD3D12RootSignature>(D3D12RootSignature);
        CommandList.SetGraphicsRootSignature(CurrentRootSignature.Get());
    }
}

void CD3D12CommandContext::SetComputePipelineState(class CRHIComputePipelineState* PipelineState)
{
    // TODO: Maybe it should be supported to unbind pipelines by setting it to nullptr
    D3D12_ERROR(PipelineState != nullptr, "PipelineState cannot be nullptr ");

    CD3D12ComputePipelineState* D3D12PipelineState = static_cast<CD3D12ComputePipelineState*>(PipelineState);
    if (D3D12PipelineState != CurrentComputePipelineState.Get())
    {
        CurrentComputePipelineState = MakeSharedRef<CD3D12ComputePipelineState>(D3D12PipelineState);
        CommandList.SetPipelineState(CurrentComputePipelineState->GetPipeline());
    }

    CD3D12RootSignature* D3D12RootSignature = D3D12PipelineState->GetRootSignature();
    if (D3D12RootSignature != CurrentRootSignature.Get())
    {
        CurrentRootSignature = MakeSharedRef<CD3D12RootSignature>(D3D12RootSignature);
        CommandList.SetComputeRootSignature(CurrentRootSignature.Get());
    }
}

void CD3D12CommandContext::Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    UNREFERENCED_VARIABLE(Shader);
    ShaderConstantsCache.Set32BitShaderConstants(reinterpret_cast<const uint32*>(Shader32BitConstants), Num32BitConstants);
}

void CD3D12CommandContext::SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetShaderResourceParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    CD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<CD3D12ShaderResourceView*>(ShaderResourceView);
    DescriptorCache.SetShaderResourceView(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
}

void CD3D12CommandContext::SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetShaderResourceParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumShaderResourceViews, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumShaderResourceViews; i++)
    {
        CD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<CD3D12ShaderResourceView*>(ShaderResourceView[i]);
        DescriptorCache.SetShaderResourceView(D3D12ShaderResourceView, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register + i);
    }
}

void CD3D12CommandContext::SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetUnorderedAccessParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    CD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<CD3D12UnorderedAccessView*>(UnorderedAccessView);
    DescriptorCache.SetUnorderedAccessView(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
}

void CD3D12CommandContext::SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetUnorderedAccessParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumUnorderedAccessViews, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
    {
        CD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<CD3D12UnorderedAccessView*>(UnorderedAccessViews[i]);
        DescriptorCache.SetUnorderedAccessView(D3D12UnorderedAccessView, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register + i);
    }
}

void CD3D12CommandContext::SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetConstantBufferParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    if (ConstantBuffer)
    {
        CD3D12ConstantBufferView& D3D12ConstantBufferView = static_cast<CD3D12ConstantBuffer*>(ConstantBuffer)->GetView();
        DescriptorCache.SetConstantBufferView(&D3D12ConstantBufferView, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
    }
    else
    {
        DescriptorCache.SetConstantBufferView(nullptr, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
    }
}

void CD3D12CommandContext::SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetConstantBufferParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumConstantBuffers, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumConstantBuffers; i++)
    {
        if (ConstantBuffers[i])
        {
            CD3D12ConstantBufferView& D3D12ConstantBufferView = static_cast<CD3D12ConstantBuffer*>(ConstantBuffers[i])->GetView();
            DescriptorCache.SetConstantBufferView(&D3D12ConstantBufferView, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
        }
        else
        {
            DescriptorCache.SetConstantBufferView(nullptr, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
        }
    }
}

void CD3D12CommandContext::SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetSamplerStateParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == 1, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    CD3D12SamplerState* D3D12SamplerState = static_cast<CD3D12SamplerState*>(SamplerState);
    DescriptorCache.SetSamplerState(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
}

void CD3D12CommandContext::SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
{
    CD3D12Shader* D3D12Shader = D3D12ShaderCast(Shader);
    D3D12_ERROR(D3D12Shader != nullptr, "Cannot bind resources to a shader that is nullptr");

    SD3D12ShaderParameter ParameterInfo = D3D12Shader->GetSamplerStateParameter(ParameterIndex);
    D3D12_ERROR(ParameterInfo.Space          == 0, "Global variables must be bound to RegisterSpace = 0");
    D3D12_ERROR(ParameterInfo.NumDescriptors == NumSamplerStates, "Trying to bind more descriptors than supported to ParameterIndex=" + ToString(ParameterIndex));

    for (uint32 i = 0; i < NumSamplerStates; i++)
    {
        CD3D12SamplerState* D3D12SamplerState = static_cast<CD3D12SamplerState*>(SamplerStates[i]);
        DescriptorCache.SetSamplerState(D3D12SamplerState, D3D12Shader->GetShaderVisibility(), ParameterInfo.Register);
    }
}

void CD3D12CommandContext::ResolveTexture(CRHITexture* Destination, CRHITexture* Source)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    CD3D12Texture* D3D12Destination = D3D12TextureCast(Destination);
    CD3D12Texture* D3D12Source      = D3D12TextureCast(Source);
    const DXGI_FORMAT DstFormat = D3D12Destination->GetDXGIFormat();
    const DXGI_FORMAT SrcFormat = D3D12Source->GetDXGIFormat();

    //TODO: For now texture must be the same format. I.e typeless does probably not work
    D3D12_ERROR(DstFormat == SrcFormat, "Destination and Source texture must have the same format");

    CommandList.ResolveSubresource(D3D12Destination->GetD3D12Resource(), D3D12Source->GetD3D12Resource(), DstFormat);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12CommandContext::UpdateBuffer(CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    if (SizeInBytes > 0)
    {
        CD3D12Buffer* D3D12Destination = D3D12BufferCast(Destination);
        UpdateBuffer(D3D12Destination->GetD3D12Resource(), OffsetInBytes, SizeInBytes, SourceData);

        CmdBatch->AddInUseResource(Destination);
    }
}

void CD3D12CommandContext::UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
    D3D12_ERROR(Destination != nullptr, "Destination cannot be nullptr");

    if (Width > 0 && Height > 0)
    {
        D3D12_ERROR(SourceData != nullptr, "SourceData cannot be nullptr");

        FlushResourceBarriers();

        CD3D12Texture* D3D12Destination = D3D12TextureCast(Destination);

        const DXGI_FORMAT NativeFormat = D3D12Destination->GetDXGIFormat();
        
        const uint32 Stride      = GetFormatStride(NativeFormat);
        const uint32 RowPitch    = ((Width * Stride) + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
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

        SourceLocation.pResource                          = GpuResourceUploader.GetGpuResource();
        SourceLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        SourceLocation.PlacedFootprint.Footprint.Format   = NativeFormat;
        SourceLocation.PlacedFootprint.Footprint.Width    = Width;
        SourceLocation.PlacedFootprint.Footprint.Height   = Height;
        SourceLocation.PlacedFootprint.Footprint.Depth    = 1;
        SourceLocation.PlacedFootprint.Footprint.RowPitch = RowPitch;
        SourceLocation.PlacedFootprint.Offset             = Allocation.ResourceOffset;

        // TODO: Miplevel may not be the correct subresource
        D3D12_TEXTURE_COPY_LOCATION DestLocation;
        CMemory::Memzero(&DestLocation);

        DestLocation.pResource        = D3D12Destination->GetD3D12Resource()->GetResource();
        DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DestLocation.SubresourceIndex = MipLevel;

        CommandList.CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

        CmdBatch->AddInUseResource(Destination);
    }
}

void CD3D12CommandContext::CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SRHICopyBufferInfo& CopyInfo)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    CD3D12Buffer* D3D12Destination = D3D12BufferCast(Destination);
    CD3D12Buffer* D3D12Source = D3D12BufferCast(Source);
    CommandList.CopyBufferRegion(D3D12Destination->GetD3D12Resource(), CopyInfo.DestinationOffset, D3D12Source->GetD3D12Resource(), CopyInfo.SourceOffset, CopyInfo.SizeInBytes);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12CommandContext::CopyTexture(CRHITexture* Destination, CRHITexture* Source)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    CD3D12Texture* D3D12Destination = D3D12TextureCast(Destination);
    CD3D12Texture* D3D12Source      = D3D12TextureCast(Source);
    CommandList.CopyResource(D3D12Destination->GetD3D12Resource(), D3D12Source->GetD3D12Resource());

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12CommandContext::CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SRHICopyTextureInfo& CopyInfo)
{
    D3D12_ERROR(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    CD3D12Texture* D3D12Destination = D3D12TextureCast(Destination);
    CD3D12Texture* D3D12Source      = D3D12TextureCast(Source);

    // Source
    D3D12_TEXTURE_COPY_LOCATION SourceLocation;
    CMemory::Memzero(&SourceLocation);

    SourceLocation.pResource        = D3D12Source->GetD3D12Resource()->GetResource();
    SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    SourceLocation.SubresourceIndex = CopyInfo.Source.SubresourceIndex;

    D3D12_BOX SourceBox;
    SourceBox.left   = CopyInfo.Source.x;
    SourceBox.right  = CopyInfo.Source.x + CopyInfo.Width;
    SourceBox.bottom = CopyInfo.Source.y;
    SourceBox.top    = CopyInfo.Source.y + CopyInfo.Height;
    SourceBox.front  = CopyInfo.Source.z;
    SourceBox.back   = CopyInfo.Source.z + CopyInfo.Depth;

    // Destination
    D3D12_TEXTURE_COPY_LOCATION DestinationLocation;
    CMemory::Memzero(&DestinationLocation);

    DestinationLocation.pResource        = D3D12Destination->GetD3D12Resource()->GetResource();
    DestinationLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestinationLocation.SubresourceIndex = CopyInfo.Destination.SubresourceIndex;

    FlushResourceBarriers();

    CommandList.CopyTextureRegion(&DestinationLocation, CopyInfo.Destination.x, CopyInfo.Destination.y, CopyInfo.Destination.z, &SourceLocation, &SourceBox);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void CD3D12CommandContext::DestroyResource(IRHIResource* Resource)
{
    CmdBatch->AddInUseResource(Resource);
}

void CD3D12CommandContext::DiscardContents(CRHITexture* Texture)
{
    // TODO: Enable regions to be discarded

    CD3D12Resource* D3D12Resource = D3D12ResourceCast(Texture);
    if (D3D12Resource)
    {
        CommandList.DiscardResource(D3D12Resource->GetResource(), nullptr);
        CmdBatch->AddInUseResource(Texture);
    }
}

void CD3D12CommandContext::BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate)
{
    D3D12_ERROR(Geometry != nullptr, "Geometry cannot be nullptr");

    FlushResourceBarriers();

    CD3D12VertexBuffer* D3D12VertexBuffer = static_cast<CD3D12VertexBuffer*>(VertexBuffer);
    CD3D12IndexBuffer*  D3D12IndexBuffer  = static_cast<CD3D12IndexBuffer*>(IndexBuffer);
    D3D12_ERROR(D3D12VertexBuffer != nullptr, "VertexBuffer cannot be nullptr");

    CD3D12RayTracingGeometry* D3D12Geometry = static_cast<CD3D12RayTracingGeometry*>(Geometry);
    D3D12Geometry->VertexBuffer = D3D12VertexBuffer;
    D3D12Geometry->IndexBuffer  = D3D12IndexBuffer;
    D3D12Geometry->Build(*this, bUpdate);

    CmdBatch->AddInUseResource(Geometry);
    CmdBatch->AddInUseResource(VertexBuffer);
    CmdBatch->AddInUseResource(IndexBuffer);
}

void CD3D12CommandContext::BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate)
{
    D3D12_ERROR(RayTracingScene != nullptr, "RayTracingScene cannot be nullptr");

    FlushResourceBarriers();

    CD3D12RayTracingScene* D3D12Scene = static_cast<CD3D12RayTracingScene*>(RayTracingScene);
    D3D12Scene->Build(*this, Instances, NumInstances, bUpdate);

    CmdBatch->AddInUseResource(RayTracingScene);
}

void CD3D12CommandContext::SetRayTracingBindings( CRHIRayTracingScene* RayTracingScene
                                                , CRHIRayTracingPipelineState* PipelineState
                                                , const SRayTracingShaderResources* GlobalResource
                                                , const SRayTracingShaderResources* RayGenLocalResources
                                                , const SRayTracingShaderResources* MissLocalResources
                                                , const SRayTracingShaderResources* HitGroupResources
                                                , uint32 NumHitGroupResources)
{
    CD3D12RayTracingScene*         D3D12Scene         = static_cast<CD3D12RayTracingScene*>(RayTracingScene);
    CD3D12RayTracingPipelineState* D3D12PipelineState = static_cast<CD3D12RayTracingPipelineState*>(PipelineState);
    D3D12_ERROR(D3D12Scene         != nullptr, "RayTracingScene cannot be nullptr");
    D3D12_ERROR(D3D12PipelineState != nullptr, "PipelineState cannot be nullptr");

    uint32 NumDescriptorsNeeded = 0;
    uint32 NumSamplersNeeded    = 0;
    if (GlobalResource)
    {
        NumDescriptorsNeeded += GlobalResource->NumResources();
        NumSamplersNeeded    += GlobalResource->NumSamplers();
    }
    if (RayGenLocalResources)
    {
        NumDescriptorsNeeded += RayGenLocalResources->NumResources();
        NumSamplersNeeded    += RayGenLocalResources->NumSamplers();
    }
    if (MissLocalResources)
    {
        NumDescriptorsNeeded += MissLocalResources->NumResources();
        NumSamplersNeeded    += MissLocalResources->NumSamplers();
    }

    for (uint32 i = 0; i < NumHitGroupResources; i++)
    {
        NumDescriptorsNeeded += HitGroupResources[i].NumResources();
        NumSamplersNeeded    += HitGroupResources[i].NumSamplers();
    }

    D3D12_ERROR( NumDescriptorsNeeded < D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT
               , "NumDescriptorsNeeded=" + ToString(NumDescriptorsNeeded) + ", but the maximum is '" + ToString(D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT) + "'");

    CD3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    if (!ResourceHeap->HasSpace(NumDescriptorsNeeded))
    {
        ResourceHeap->AllocateFreshHeap();
    }

    D3D12_ERROR( NumSamplersNeeded < D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT
               , "NumDescriptorsNeeded=" + ToString(NumSamplersNeeded) + ", but the maximum is '" + ToString(D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT) + "'");

    CD3D12OnlineDescriptorHeap* SamplerHeap = CmdBatch->GetOnlineSamplerDescriptorHeap();
    if (!SamplerHeap->HasSpace(NumSamplersNeeded))
    {
        SamplerHeap->AllocateFreshHeap();
    }

    if (!D3D12Scene->BuildBindingTable(*this, D3D12PipelineState, ResourceHeap, SamplerHeap, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources))
    {
        D3D12_ERROR_ALWAYS("[D3D12CommandContext]: FAILED to Build Shader Binding Table");
    }

    if (GlobalResource)
    {
        if (!GlobalResource->ConstantBuffers.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ConstantBuffers.Size(); i++)
            {
                CD3D12ConstantBufferView& D3D12ConstantBufferView = static_cast<CD3D12ConstantBuffer*>(GlobalResource->ConstantBuffers[i])->GetView();
                DescriptorCache.SetConstantBufferView(&D3D12ConstantBufferView, ShaderVisibility_All, i);
            }
        }
        if (!GlobalResource->ShaderResourceViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ShaderResourceViews.Size(); i++)
            {
                CD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<CD3D12ShaderResourceView*>(GlobalResource->ShaderResourceViews[i]);
                DescriptorCache.SetShaderResourceView(D3D12ShaderResourceView, ShaderVisibility_All, i);
            }
        }
        if (!GlobalResource->UnorderedAccessViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->UnorderedAccessViews.Size(); i++)
            {
                CD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<CD3D12UnorderedAccessView*>(GlobalResource->UnorderedAccessViews[i]);
                DescriptorCache.SetUnorderedAccessView(D3D12UnorderedAccessView, ShaderVisibility_All, i);
            }
        }
        if (!GlobalResource->SamplerStates.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->SamplerStates.Size(); i++)
            {
                CD3D12SamplerState* DxSampler = static_cast<CD3D12SamplerState*>(GlobalResource->SamplerStates[i]);
                DescriptorCache.SetSamplerState(DxSampler, ShaderVisibility_All, i);
            }
        }
    }

    ID3D12GraphicsCommandList4* DXRCommandList = CommandList.GetDXRCommandList();

    CD3D12RootSignature* GlobalRootSignature = D3D12PipelineState->GetGlobalRootSignature();
    CurrentRootSignature = MakeSharedRef<CD3D12RootSignature>(GlobalRootSignature);

    DXRCommandList->SetComputeRootSignature(CurrentRootSignature->GetRootSignature());

    DescriptorCache.CommitComputeDescriptors(CommandList, CmdBatch, CurrentRootSignature.Get());
}

void CD3D12CommandContext::GenerateMips(CRHITexture* Texture)
{
    CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
    D3D12_ERROR(D3D12Texture != nullptr, "Texture cannot be nullptr");

    D3D12_RESOURCE_DESC Desc = D3D12Texture->GetD3D12Resource()->GetDesc();
    Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_ERROR(Desc.MipLevels > 1, "MipLevels must be more than one in order to generate any MipLevels");

    // TODO: Create this placed from a Heap? See what performance is 
    TSharedRef<CD3D12Resource> StagingTexture = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12Texture->GetD3D12Resource()->GetHeapType());
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

    SrvDesc.Format                  = Desc.Format;
    SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (bIsTextureCube)
    {
        SrvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        SrvDesc.TextureCube.MipLevels           = Desc.MipLevels;
        SrvDesc.TextureCube.MostDetailedMip     = 0;
        SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        SrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        SrvDesc.Texture2D.MipLevels       = Desc.MipLevels;
        SrvDesc.Texture2D.MostDetailedMip = 0;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
    CMemory::Memzero(&UavDesc);

    UavDesc.Format = Desc.Format;
    if (bIsTextureCube)
    {
        UavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UavDesc.Texture2DArray.ArraySize       = 6;
        UavDesc.Texture2DArray.FirstArraySlice = 0;
        UavDesc.Texture2DArray.PlaneSlice      = 0;
    }
    else
    {
        UavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        UavDesc.Texture2D.PlaneSlice = 0;
    }

    const uint32 MipLevelsPerDispatch     = 4;
    const uint32 UavDescriptorHandleCount = NMath::AlignUp<uint32>(Desc.MipLevels, MipLevelsPerDispatch);
    const uint32 NumDispatches            = UavDescriptorHandleCount / MipLevelsPerDispatch;

    CD3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();

    // Allocate an extra handle for SRV
    const uint32 StartDescriptorHandleIndex = ResourceHeap->AllocateHandles(UavDescriptorHandleCount + 1);

    const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(StartDescriptorHandleIndex);
    GetDevice()->CreateShaderResourceView(D3D12Texture->GetD3D12Resource()->GetResource(), &SrvDesc, SrvHandle_CPU);

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
    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushResourceBarriers();

    CommandList.CopyResource(StagingTexture.Get(), D3D12Texture->GetD3D12Resource());

    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    FlushResourceBarriers();

    if (bIsTextureCube)
    {
        TSharedRef<CD3D12ComputePipelineState> PipelineState = GD3D12Instance->GetGenerateMipsPipelineTexureCube();
        CommandList.SetPipelineState(PipelineState->GetPipeline());
        CommandList.SetComputeRootSignature(PipelineState->GetRootSignature());
    }
    else
    {
        TSharedRef<CD3D12ComputePipelineState> PipelineState = GD3D12Instance->GetGenerateMipsPipelineTexure2D();
        CommandList.SetPipelineState(PipelineState->GetPipeline());
        CommandList.SetComputeRootSignature(PipelineState->GetRootSignature());
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE SrvHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(StartDescriptorHandleIndex);

    ID3D12DescriptorHeap* OnlineResourceHeap = ResourceHeap->GetHeap()->GetHeap();
    CommandList.SetDescriptorHeaps(&OnlineResourceHeap, 1);

    struct SConstantBuffer
    {
        uint32   SrcMipLevel;
        uint32   NumMipLevels;
        CVector2 TexelSize;
    } ConstantData;

    uint32 DstWidth  = static_cast<uint32>(Desc.Width);
    uint32 DstHeight = Desc.Height;
    ConstantData.SrcMipLevel = 0;

    const uint32 ThreadsZ = bIsTextureCube ? 6 : 1;

    uint32 RemainingMiplevels = Desc.MipLevels;
    for (uint32 i = 0; i < NumDispatches; i++)
    {
        ConstantData.TexelSize    = CVector2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));
        ConstantData.NumMipLevels = NMath::Min<uint32>(4, RemainingMiplevels);

        CommandList.SetComputeRoot32BitConstants(&ConstantData, 4, 0, 0);
        CommandList.SetComputeRootDescriptorTable(SrvHandle_GPU, 1);

        const uint32 GPUDescriptorHandleIndex = i * MipLevelsPerDispatch;

        const D3D12_GPU_DESCRIPTOR_HANDLE UavHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(UavStartDescriptorHandleIndex + GPUDescriptorHandleIndex);
        CommandList.SetComputeRootDescriptorTable(UavHandle_GPU, 2);

        constexpr uint32 ThreadCount = 8;

        const uint32 ThreadsX = NMath::DivideByMultiple(DstWidth, ThreadCount);
        const uint32 ThreadsY = NMath::DivideByMultiple(DstHeight, ThreadCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, ThreadsZ);

        UnorderedAccessBarrier(StagingTexture.Get());

        TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        // TODO: Copy only miplevels (Maybe faster?)
        CommandList.CopyResource(D3D12Texture->GetD3D12Resource(), StagingTexture.Get());

        TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        FlushResourceBarriers();

        DstWidth  = DstWidth / 16;
        DstHeight = DstHeight / 16;

        ConstantData.SrcMipLevel += 3;
        RemainingMiplevels       -= MipLevelsPerDispatch;
    }

    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushResourceBarriers();

    CmdBatch->AddInUseResource(Texture);
    CmdBatch->AddInUseResource(StagingTexture.Get());
}

void CD3D12CommandContext::TransitionTexture(CRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
    TransitionResource(D3D12Texture->GetD3D12Resource(), D3D12BeforeState, D3D12AfterState);

    CmdBatch->AddInUseResource(Texture);
}

void CD3D12CommandContext::TransitionBuffer(CRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    CD3D12Buffer* D3D12Buffer = D3D12BufferCast(Buffer);
    TransitionResource(D3D12Buffer->GetD3D12Resource(), D3D12BeforeState, D3D12AfterState);

    CmdBatch->AddInUseResource(Buffer);
}

void CD3D12CommandContext::UnorderedAccessTextureBarrier(CRHITexture* Texture)
{
    CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
    UnorderedAccessBarrier(D3D12Texture->GetD3D12Resource());

    CmdBatch->AddInUseResource(Texture);
}

void CD3D12CommandContext::UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)
{
    CD3D12Buffer* D3D12Buffer = D3D12BufferCast(Buffer);
    UnorderedAccessBarrier(D3D12Buffer->GetD3D12Resource());

    CmdBatch->AddInUseResource(Buffer);
}

void CD3D12CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    FlushResourceBarriers();

    if (VertexCount)
    {
        ShaderConstantsCache.CommitGraphics(CommandList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CommandList, CmdBatch, CurrentRootSignature.Get());

        CommandList.DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
    }
}

void CD3D12CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    FlushResourceBarriers();

    if (IndexCount)
    {
        ShaderConstantsCache.CommitGraphics(CommandList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CommandList, CmdBatch, CurrentRootSignature.Get());

        CommandList.DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
    }
}

void CD3D12CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    if (VertexCountPerInstance > 0 && InstanceCount > 0)
    {
        ShaderConstantsCache.CommitGraphics(CommandList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CommandList, CmdBatch, CurrentRootSignature.Get());

        CommandList.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }
}

void CD3D12CommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    if (IndexCountPerInstance > 0 && InstanceCount > 0)
    {
        ShaderConstantsCache.CommitGraphics(CommandList, CurrentRootSignature.Get());
        DescriptorCache.CommitGraphicsDescriptors(CommandList, CmdBatch, CurrentRootSignature.Get());

        CommandList.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }
}

void CD3D12CommandContext::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    FlushResourceBarriers();

    if (ThreadGroupCountX > 0 || ThreadGroupCountY > 0 || ThreadGroupCountZ > 0)
    {
        ShaderConstantsCache.CommitCompute(CommandList, CurrentRootSignature.Get());
        DescriptorCache.CommitComputeDescriptors(CommandList, CmdBatch, CurrentRootSignature.Get());

        CommandList.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }
}

void CD3D12CommandContext::DispatchRays(CRHIRayTracingScene* RayTracingScene, CRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    CD3D12RayTracingScene* D3D12Scene = static_cast<CD3D12RayTracingScene*>(RayTracingScene);
    D3D12_ERROR(D3D12Scene != nullptr, "RayTracingScene cannot be nullptr");

    CD3D12RayTracingPipelineState* D3D12PipelineState = static_cast<CD3D12RayTracingPipelineState*>(PipelineState);
    D3D12_ERROR(D3D12PipelineState != nullptr, "PipelineState cannot be nullptr");

    ID3D12GraphicsCommandList4* DXRCommandList = CommandList.GetDXRCommandList();
    D3D12_ERROR(DXRCommandList != nullptr, "DXRCommandList is nullptr, DXR is not supported");

    FlushResourceBarriers();

    if (Width > 0 || Height > 0 || Depth > 0)
    {
        D3D12_DISPATCH_RAYS_DESC RayDispatchDesc;
        CMemory::Memzero(&RayDispatchDesc);

        RayDispatchDesc.RayGenerationShaderRecord = D3D12Scene->GetRayGenShaderRecord();
        RayDispatchDesc.MissShaderTable           = D3D12Scene->GetMissShaderTable();
        RayDispatchDesc.HitGroupTable             = D3D12Scene->GetHitGroupTable();

        RayDispatchDesc.Width  = Width;
        RayDispatchDesc.Height = Height;
        RayDispatchDesc.Depth  = Depth;

        DXRCommandList->SetPipelineState1(D3D12PipelineState->GetStateObject());
        DXRCommandList->DispatchRays(&RayDispatchDesc);
    }
}

void CD3D12CommandContext::ClearState()
{
    Flush();

    InternalClearState();
}

void CD3D12CommandContext::Flush()
{
    const uint64 NewFenceValue = ++FenceValue;
    if (!CommandQueue.SignalFence(Fence, NewFenceValue))
    {
        return;
    }

    Fence.WaitForValue(FenceValue);

    for (CD3D12CommandBatch& Batch : CmdBatches)
    {
        Batch.Reset();
    }
}

void CD3D12CommandContext::InsertMarker(const String& Message)
{
    if (ND3D12Functions::SetMarkerOnCommandList)
    {
        ND3D12Functions::SetMarkerOnCommandList(CommandList.GetGraphicsCommandList(), PIX_COLOR(255, 255, 255), Message.CStr());
    }
}

void CD3D12CommandContext::BeginExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetGraphicsAnalysisInterface();
    if (GraphicsAnalysis && !bIsCapturing)
    {
        GraphicsAnalysis->BeginCapture();
        bIsCapturing = true;
    }
}

void CD3D12CommandContext::EndExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetGraphicsAnalysisInterface();
    if (GraphicsAnalysis && bIsCapturing)
    {
        GraphicsAnalysis->EndCapture();
        bIsCapturing = false;
    }
}

void CD3D12CommandContext::InternalClearState()
{
    DescriptorCache.Reset();
    ShaderConstantsCache.Reset();

    CurrentGraphicsPipelineState.Reset();
    CurrentRootSignature.Reset();
    CurrentComputePipelineState.Reset();

    CurrentPrimitiveTolpology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
