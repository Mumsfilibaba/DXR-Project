#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12Core.h"
#include "D3D12Shader.h"
#include "D3D12Interface.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12PipelineState.h"
#include "D3D12RayTracing.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12TimestampQuery.h"
#include "D3D12CommandContext.h"
#include "DynamicD3D12.h"
#include "D3D12Viewport.h"

#include "Core/Math/Vector2.h"
#include "Core/Debug/Profiler/FrameProfiler.h"

#include <pix.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ResourceBarrierBatcher

void FD3D12ResourceBarrierBatcher::AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
    Check(Resource != nullptr);

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
        FMemory::Memzero(&Barrier);

        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = Resource;
        Barrier.Transition.StateAfter  = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        Barriers.Emplace(Barrier);
    }
}

void FD3D12ResourceBarrierBatcher::AddUnorderedAccessBarrier(ID3D12Resource* Resource)
{
    Check(Resource != nullptr);

    // Make sure we are not already have UAV barrier for this resource
    for (TArray<D3D12_RESOURCE_BARRIER>::IteratorType It = Barriers.StartIterator(); It != Barriers.EndIterator(); It++)
    {
        if ((*It).Type == D3D12_RESOURCE_BARRIER_TYPE_UAV)
        {
            if ((*It).UAV.pResource == Resource)
            {
                It = Barriers.RemoveAt(It);
                return;
            }
        }
    }

    D3D12_RESOURCE_BARRIER Barrier;
    FMemory::Memzero(&Barrier);

    Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.UAV.pResource = Resource;

    Barriers.Emplace(Barrier);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12GPUResourceUploader

FD3D12GPUResourceUploader::FD3D12GPUResourceUploader(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , MappedMemory(nullptr)
    , SizeInBytes(0)
    , OffsetInBytes(0)
    , Resource(nullptr)
    , GarbageResources()
{ }

bool FD3D12GPUResourceUploader::Reserve(uint32 InSizeInBytes)
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
    FMemory::Memzero(&HeapProperties);

    HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC Desc;
    FMemory::Memzero(&Desc);

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

    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommittedResource( &HeapProperties
                                                                           , D3D12_HEAP_FLAG_NONE
                                                                           , &Desc
                                                                           , D3D12_RESOURCE_STATE_GENERIC_READ
                                                                           , nullptr
                                                                           , IID_PPV_ARGS(&Resource));
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

void FD3D12GPUResourceUploader::Reset()
{
    constexpr uint32 MAX_RESERVED_GARBAGE_RESOURCES = 5;
    constexpr uint32 NEW_RESERVED_GARBAGE_RESOURCES = 2;

    // Clear garbage resource, and release memory we do not need
    GarbageResources.Clear();
    if (GarbageResources.GetCapacity() >= MAX_RESERVED_GARBAGE_RESOURCES)
    {
        GarbageResources.Reserve(NEW_RESERVED_GARBAGE_RESOURCES);
    }

    // Reset memory offset
    OffsetInBytes = 0;
}

FD3D12UploadAllocation FD3D12GPUResourceUploader::LinearAllocate(uint32 InSizeInBytes)
{
    constexpr uint32 EXTRA_BYTES_ALLOCATED = 1024;

    const uint32 NeededSize = OffsetInBytes + InSizeInBytes;
    if (NeededSize > SizeInBytes)
    {
        Reserve(NeededSize + EXTRA_BYTES_ALLOCATED);
    }

    FD3D12UploadAllocation Allocation;
    Allocation.MappedPtr      = MappedMemory + OffsetInBytes;
    Allocation.ResourceOffset = OffsetInBytes;
    OffsetInBytes            += InSizeInBytes;
    return Allocation;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandBatch

FD3D12CommandBatch::FD3D12CommandBatch(FD3D12Device* InDevice)
    : Device(InDevice)
    , CmdAllocator(InDevice)
    , GpuResourceUploader(InDevice)
    , OnlineResourceDescriptorHeap(nullptr)
    , OnlineSamplerDescriptorHeap(nullptr)
    , Resources()
{ }

bool FD3D12CommandBatch::Initialize(uint32 Index)
{
    // TODO: Do not have D3D12_COMMAND_LIST_TYPE_DIRECT directly
    if (!CmdAllocator.Create(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    const uint32 ResourceCount = 100000;
    const uint32 SamplerCount  = 200;

    OnlineResourceDescriptorHeap = dbg_new FD3D12OnlineDescriptorManager(Device, Device->GetGlobalResourceHeap(), Index * ResourceCount, ResourceCount);
    Check(OnlineResourceDescriptorHeap != nullptr);

    OnlineSamplerDescriptorHeap = dbg_new FD3D12OnlineDescriptorManager(Device, Device->GetGlobalSamplerHeap(), Index * SamplerCount, SamplerCount);
    Check(OnlineResourceDescriptorHeap != nullptr);

    GpuResourceUploader.Reserve(1024);
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandContextState

FD3D12CommandContextState::FD3D12CommandContextState(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , DescriptorCache(InDevice)
    , bIsReady(false)
    , bIsCapturing(false)
    , bIsRenderPassActive(false)
{ }

bool FD3D12CommandContextState::Initialize()
{
    if (!DescriptorCache.Initialize())
    {
        D3D12_ERROR("Failed to initialize DescriptorCache");
        return false;
    }

    return true;
}

void FD3D12CommandContextState::ApplyGraphics(FD3D12CommandList& CommandList, FD3D12CommandBatch* Batch)
{
    // VertexBuffer and IndexBuffer
    if (Graphics.bBindVertexBuffers)
    {
        DescriptorCache.SetVertexBuffers(Graphics.VBCache);
        Graphics.bBindVertexBuffers = false;
    }

    if (Graphics.bBindIndexBuffer)
    {
        DescriptorCache.SetIndexBuffer(Graphics.IBCache);
        Graphics.bBindIndexBuffer = false;
    }

    // Scissor and Viewport
    if (Graphics.bBindViewports)
    {
        CommandList.RSSetViewports(Graphics.Viewports, Graphics.NumViewports);
        Graphics.bBindViewports = false;
    }

    if (Graphics.bBindScissorRects)
    {
        CommandList.RSSetScissorRects(Graphics.ScissorRects, Graphics.NumScissor);
        Graphics.bBindScissorRects = false;
    }

    // Topology
    if (Graphics.bBindPrimitiveTopology)
    {
        CommandList.IASetPrimitiveTopology(Graphics.PrimitiveTopology);
        Graphics.bBindPrimitiveTopology = false;
    }

    // BlendFactor
    if (Graphics.bBindBlendFactor)
    {
        CommandList.OMSetBlendFactor(Graphics.BlendFactor.GetData());
        Graphics.bBindBlendFactor = false;
    }

    // Pipeline
    FD3D12RootSignature* CurrentRootSignture = Graphics.PipelineState->GetRootSignature();
    if (Graphics.bBindPipeline)
    {
        CommandList.SetPipelineState(Graphics.PipelineState->GetD3D12PipelineState());
        CommandList.SetGraphicsRootSignature(CurrentRootSignture);

        Graphics.bBindPipeline = false;
    }

    // Descriptors
    DescriptorCache.PrepareGraphicsDescriptors(Batch, CurrentRootSignture, FD3D12PipelineStageMask::BasicGraphicsMask());

    // Constants
    ShaderConstantsCache.CommitGraphics(CommandList, CurrentRootSignture);

    // RenderTargets, DepthStencil and ShadingRate
    if (Graphics.bBindRenderTargets)
    {
        DescriptorCache.SetRenderTargets(Graphics.RTCache, Graphics.DepthStencil);

        FD3D12VariableRateShadingDesc VRSSupport = GetDevice()->GetVariableRateShadingDesc();
        if (VRSSupport.IsSupported())
        {
            if (Graphics.ShadingRateTexture)
            {
                CommandList.RSSetShadingRateImage(Graphics.ShadingRateTexture->GetResource()->GetD3D12Resource());
            }
            else
            {
                D3D12_SHADING_RATE_COMBINER Combiners[] =
                {
                    D3D12_SHADING_RATE_COMBINER_OVERRIDE,
                    D3D12_SHADING_RATE_COMBINER_OVERRIDE,
                };

                CommandList.RSSetShadingRateImage(nullptr);
                CommandList.RSSetShadingRate(Graphics.ShadingRate, Combiners);
            }
        }

        Graphics.ShadingRateTexture = nullptr;
        Graphics.ShadingRate        = D3D12_SHADING_RATE_1X1;
        Graphics.bBindRenderTargets = false;
    }
}

void FD3D12CommandContextState::ApplyCompute(FD3D12CommandList& CommandList, FD3D12CommandBatch* Batch)
{
    // Pipeline
    FD3D12RootSignature* CurrentRootSignture = Compute.PipelineState->GetRootSignature();
    if (Compute.bBindPipeline)
    {
        CommandList.SetPipelineState(Compute.PipelineState->GetD3D12PipelineState());
        CommandList.SetComputeRootSignature(CurrentRootSignture);

        Compute.bBindPipeline = false;
    }

    // Descriptors
    DescriptorCache.PrepareComputeDescriptors(Batch, CurrentRootSignture);

    // Constants
    ShaderConstantsCache.CommitCompute(CommandList, CurrentRootSignture);
}

void FD3D12CommandContextState::ClearGraphics()
{
    Graphics.VBCache.Clear();
    Graphics.IBCache.Clear();

    Graphics.PipelineState.Reset();
    Graphics.PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    Graphics.ShadingRateTexture.Reset();
    Graphics.RTCache.Clear();
    Graphics.DepthStencil = nullptr;

    Graphics.bBindRenderTargets     = true;
    Graphics.bBindBlendFactor       = true;
    Graphics.bBindPrimitiveTopology = true;
    Graphics.bBindPipeline          = true;
    Graphics.bBindVertexBuffers     = true;
    Graphics.bBindIndexBuffer       = true;
    Graphics.bBindScissorRects      = true;
    Graphics.bBindViewports         = true;
}

void FD3D12CommandContextState::ClearCompute()
{
    Compute.PipelineState.Reset();

    Compute.bBindPipeline = true;
}

void FD3D12CommandContextState::SetVertexBuffer(FD3D12VertexBuffer* VertexBuffer, uint32 Slot)
{
    FD3D12Resource* CurrentResource = Graphics.VBCache.VBResources[Slot];
    if (!VertexBuffer)
    {
        FMemory::Memzero(&Graphics.VBCache.VBViews[Slot]);
        Graphics.VBCache.VBResources[Slot] = nullptr;
    }
    else
    {
        Graphics.VBCache.VBViews[Slot]     = VertexBuffer->GetView();
        Graphics.VBCache.VBResources[Slot] = VertexBuffer->GetD3D12Resource();
    }

    if (Graphics.VBCache.VBResources[Slot] != CurrentResource)
    {
        Graphics.bBindVertexBuffers = true;
    }
}

void FD3D12CommandContextState::SetIndexBuffer(FD3D12IndexBuffer* IndexBuffer)
{
    FD3D12Resource* CurrentResource = Graphics.IBCache.IBResource;
    if (IndexBuffer)
    {
        Graphics.IBCache.IBView     = IndexBuffer->GetView();
        Graphics.IBCache.IBResource = IndexBuffer->GetD3D12Resource();
    }
    else
    {
        Graphics.IBCache.Clear();
    }

    if (Graphics.IBCache.IBResource != CurrentResource)
    {
        Graphics.bBindIndexBuffer = true;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandContext

FD3D12CommandContext::FD3D12CommandContext(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : IRHICommandContext()
    , FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandList(nullptr)
    , Fence(InDevice)
    , CmdBatches()
    , BarrierBatcher()
    , State(InDevice)
{ }

FD3D12CommandContext::~FD3D12CommandContext()
{
    Flush();
}

bool FD3D12CommandContext::Initialize()
{
    for (uint32 Index = 0; Index < D3D12_NUM_BACK_BUFFERS; ++Index)
    {
        FD3D12CommandBatch& Batch = CmdBatches.Emplace(GetDevice());
        if (!Batch.Initialize(Index))
        {
            D3D12_ERROR("Failed to initialize D3D12CommandBatch");
            return false;
        }
    }

    CommandList = GetDevice()->GetCommandListManager(ED3D12CommandQueueType::Direct)->ObtainCommandList(
        CmdBatches[0].GetCommandAllocator(),
        nullptr);

    if (!CommandList)
    {
        D3D12_ERROR("Failed to initialize CommandList");
        return false;
    }

    FenceValue = 0;
    if (!Fence.Initialize(FenceValue))
    {
        D3D12_ERROR("Failed to initialize Fence");
        return false;
    }

    if (!State.Initialize())
    {
        D3D12_ERROR("Failed to initialize ContextState");
        return false;
    }

    return true;
}

void FD3D12CommandContext::UpdateBuffer(FD3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    D3D12_ERROR_COND(Resource != nullptr, "Resource cannot be nullptr");

    if (SizeInBytes)
    {
        D3D12_ERROR_COND(SourceData != nullptr, "SourceData cannot be nullptr");

        FlushResourceBarriers();

        FD3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();

        FD3D12UploadAllocation Allocation = GpuResourceUploader.LinearAllocate((uint32)SizeInBytes);
        FMemory::Memcpy(Allocation.MappedPtr, SourceData, SizeInBytes);

        CommandList->CopyBufferRegion(Resource->GetD3D12Resource(), OffsetInBytes, GpuResourceUploader.GetGpuResource(), Allocation.ResourceOffset, SizeInBytes);
        CmdBatch->AddInUseResource(Resource);
    }
}

void FD3D12CommandContext::StartContext()
{
    // Lock to the thread that started the context
    CommandContextCS.Lock();

    AquireCommandList();
}

void FD3D12CommandContext::FinishContext()
{
    EndCommandList();

    // Unlock from the thread that started the context
    CommandContextCS.Unlock();
}

void FD3D12CommandContext::BeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
    FD3D12TimestampQuery* D3D12TimestampQuery = static_cast<FD3D12TimestampQuery*>(TimestampQuery);
    D3D12_ERROR_COND(D3D12TimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* DxCmdList = CommandList->GetGraphicsCommandList();
    D3D12TimestampQuery->BeginQuery(DxCmdList, Index);

    ResolveQueries.PushUnique(MakeSharedRef<FD3D12TimestampQuery>(D3D12TimestampQuery));
}

void FD3D12CommandContext::EndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
    FD3D12TimestampQuery* D3D12TimestampQuery = static_cast<FD3D12TimestampQuery*>(TimestampQuery);
    D3D12_ERROR_COND(D3D12TimestampQuery != nullptr, "TimestampQuery cannot be nullptr");

    ID3D12GraphicsCommandList* D3D12CmdList = CommandList->GetGraphicsCommandList();
    D3D12TimestampQuery->EndQuery(D3D12CmdList, Index);
}

void FD3D12CommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    FlushResourceBarriers();

    FD3D12Texture* D3D12Texture = GetD3D12Texture(RenderTargetView.Texture);
    D3D12_ERROR_COND(D3D12Texture != nullptr, "Texture cannot be nullptr when clearing the surface");

    FD3D12RenderTargetView* D3D12RenderTargetView = D3D12Texture->GetOrCreateRTV(RenderTargetView);
    Check(D3D12RenderTargetView != nullptr);

    CommandList->ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), ClearColor.GetData(), 0, nullptr);
}

void FD3D12CommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    FlushResourceBarriers();

    FD3D12Texture* D3D12Texture = GetD3D12Texture(DepthStencilView.Texture);
    D3D12_ERROR_COND(D3D12Texture != nullptr, "Texture cannot be nullptr when clearing the surface");

    FD3D12DepthStencilView* D3D12DepthStencilView = D3D12Texture->GetOrCreateDSV(DepthStencilView);
    Check(D3D12DepthStencilView != nullptr);

    CommandList->ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Depth, Stencil);
}

void FD3D12CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    D3D12_ERROR_COND(UnorderedAccessView != nullptr, "UnorderedAccessView cannot be nullptr when clearing the surface");

    FlushResourceBarriers();

    FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(UnorderedAccessView);
    CmdBatch->AddInUseResource(D3D12UnorderedAccessView);

    FD3D12OnlineDescriptorManager* OnlineDescriptorHeap = CmdBatch->GetResourceDescriptorManager();
    const uint32 OnlineDescriptorHandleIndex = OnlineDescriptorHeap->AllocateHandles(1);

    const D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle    = D3D12UnorderedAccessView->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle_CPU = OnlineDescriptorHeap->GetCPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    GetDevice()->GetD3D12Device()->CopyDescriptorsSimple(1, OnlineHandle_CPU, OfflineHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandle_GPU = OnlineDescriptorHeap->GetGPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    CommandList->ClearUnorderedAccessViewFloat(OnlineHandle_GPU, D3D12UnorderedAccessView, ClearColor.GetData());
}

void FD3D12CommandContext::BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer)
{
    D3D12_ERROR_COND(State.bIsRenderPassActive == false, "A RenderPass is already active");

    FlushResourceBarriers();

    // RenderTargetView
    const uint32 NumRenderTargets = RenderPassInitializer.NumRenderTargets;
    State.Graphics.RTCache.NumRenderTargets = NumRenderTargets;

    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& RenderTargetView = RenderPassInitializer.RenderTargets[Index];

        FD3D12Texture* D3D12Texture = GetD3D12Texture(RenderTargetView.Texture);
        if (D3D12Texture)
        {
            FD3D12RenderTargetView* D3D12RenderTargetView = D3D12Texture->GetOrCreateRTV(RenderTargetView);
            Check(D3D12RenderTargetView != nullptr);

            // Clear the RenderTarget here, since we expect it to be cleared when the RenderPass begin, however
            // it is not certain that there will be a call to draw inside of the RenderPass
            if (RenderTargetView.LoadAction == EAttachmentLoadAction::Clear)
            {
                CommandList->ClearRenderTargetView(D3D12RenderTargetView->GetOfflineHandle(), RenderTargetView.ClearValue.GetData(), 0, nullptr);
            }
            
            State.Graphics.RTCache.SetRenderTarget(D3D12RenderTargetView, Index);
        }
        else
        {
            State.Graphics.RTCache.SetRenderTarget(nullptr, Index);
        }
    }

    // DepthStencil
    const FRHIDepthStencilView& DepthStencilView = RenderPassInitializer.DepthStencilView;
    if (DepthStencilView.Texture)
    {
        FD3D12Texture*          D3D12Texture = GetD3D12Texture(DepthStencilView.Texture);
        FD3D12DepthStencilView* D3D12DepthStencilView = D3D12Texture->GetOrCreateDSV(DepthStencilView);
        Check(D3D12DepthStencilView != nullptr);

        // Clear the RenderTarget here, since we expect it to be cleared when the RenderPass begin, however
        // it is not certain that there will be a call to draw inside of the RenderPass
        if (DepthStencilView.LoadAction == EAttachmentLoadAction::Clear)
        {
            const D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
            CommandList->ClearDepthStencilView(D3D12DepthStencilView->GetOfflineHandle(), ClearFlags, DepthStencilView.ClearValue.Depth, DepthStencilView.ClearValue.Stencil);
        }

        State.Graphics.DepthStencil = D3D12DepthStencilView;
    }
    else
    {
        State.Graphics.DepthStencil = nullptr;
    }

    // ShadingRate
    FD3D12Texture* D3D12Texture       = GetD3D12Texture(RenderPassInitializer.ShadingRateTexture);
    State.Graphics.ShadingRateTexture = MakeSharedRef<FD3D12Texture>(D3D12Texture);
    State.Graphics.ShadingRate        = ConvertShadingRate(RenderPassInitializer.StaticShadingRate);
    State.Graphics.bBindRenderTargets = true;
    State.bIsRenderPassActive         = true;
}

void FD3D12CommandContext::EndRenderPass()
{
    D3D12_ERROR_COND(State.bIsRenderPassActive == true, "No RenderPass is active");
    State.bIsRenderPassActive = false;
}

void FD3D12CommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
{
    D3D12_VIEWPORT& Viewport = State.Graphics.Viewports[0];
    Viewport.Width    = Width;
    Viewport.Height   = Height;
    Viewport.MaxDepth = MaxDepth;
    Viewport.MinDepth = MinDepth;
    Viewport.TopLeftX = x;
    Viewport.TopLeftY = y;

    State.Graphics.NumViewports   = 1;
    State.Graphics.bBindViewports = true;
}

void FD3D12CommandContext::SetScissorRect(float Width, float Height, float x, float y)
{
    D3D12_RECT& ScissorRect = State.Graphics.ScissorRects[0];
    ScissorRect.top    = LONG(y);
    ScissorRect.bottom = LONG(Height);
    ScissorRect.left   = LONG(x);
    ScissorRect.right  = LONG(Width);

    State.Graphics.NumScissor        = 1;
    State.Graphics.bBindScissorRects = true;
}

void FD3D12CommandContext::SetBlendFactor(const FVector4& Color)
{
    State.Graphics.BlendFactor      = Color;
    State.Graphics.bBindBlendFactor = true;
}

void FD3D12CommandContext::SetPrimitiveTopology(EPrimitiveTopology InPrimitveTopology)
{
    const D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = ConvertPrimitiveTopology(InPrimitveTopology);
    if (State.Graphics.PrimitiveTopology != PrimitiveTopology)
    {
        State.Graphics.PrimitiveTopology      = PrimitiveTopology;
        State.Graphics.bBindPrimitiveTopology = true;
    }
}

void FD3D12CommandContext::SetVertexBuffers(const TArrayView<FRHIVertexBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    D3D12_ERROR_COND(
        (BufferSlot + InVertexBuffers.GetSize()) < D3D12_MAX_VERTEX_BUFFER_SLOTS,
        "Trying to set a VertexBuffer to an invalid slot");

    for (int32 Index = 0; Index < InVertexBuffers.GetSize(); ++Index)
    {
        FD3D12VertexBuffer* D3D12VertexBuffer = static_cast<FD3D12VertexBuffer*>(InVertexBuffers[Index]);
        State.SetVertexBuffer(D3D12VertexBuffer, BufferSlot + Index);
    }

    State.Graphics.VBCache.NumVertexBuffers = NMath::Max(State.Graphics.VBCache.NumVertexBuffers, BufferSlot + InVertexBuffers.GetSize());
}

void FD3D12CommandContext::SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)
{
    FD3D12IndexBuffer* D3D12IndexBuffer = static_cast<FD3D12IndexBuffer*>(IndexBuffer);
    State.SetIndexBuffer(D3D12IndexBuffer);
}

void FD3D12CommandContext::SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FD3D12GraphicsPipelineState* D3D12PipelineState = static_cast<FD3D12GraphicsPipelineState*>(PipelineState);
    if (State.Graphics.PipelineState != D3D12PipelineState)
    {
        State.Graphics.PipelineState = MakeSharedRef<FD3D12GraphicsPipelineState>(D3D12PipelineState);
        State.Graphics.bBindPipeline = true;
    }
}

void FD3D12CommandContext::SetComputePipelineState(class FRHIComputePipelineState* PipelineState)
{
    FD3D12ComputePipelineState* D3D12PipelineState = static_cast<FD3D12ComputePipelineState*>(PipelineState);
    if (State.Compute.PipelineState != D3D12PipelineState)
    {
        State.Compute.PipelineState = MakeSharedRef<FD3D12ComputePipelineState>(D3D12PipelineState);
        State.Compute.bBindPipeline = true;
    }
}

void FD3D12CommandContext::Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    UNREFERENCED_VARIABLE(Shader);
    State.ShaderConstantsCache.Set32BitShaderConstants(reinterpret_cast<const uint32*>(Shader32BitConstants), Num32BitConstants);
}

void FD3D12CommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetShaderResourceParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(ShaderResourceView);
    State.DescriptorCache.SetShaderResourceView(D3D12Shader->GetShaderVisibility(), D3D12ShaderResourceView, ParameterInfo.Register);
}

void FD3D12CommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetShaderResourceParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    for (int32 Index = 0; Index < InShaderResourceViews.GetSize(); ++Index)
    {
        FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(InShaderResourceViews[Index]);
        State.DescriptorCache.SetShaderResourceView(D3D12Shader->GetShaderVisibility(), D3D12ShaderResourceView, ParameterInfo.Register + Index);
    }
}

void FD3D12CommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetUnorderedAccessParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(UnorderedAccessView);
    State.DescriptorCache.SetUnorderedAccessView(D3D12Shader->GetShaderVisibility(), D3D12UnorderedAccessView, ParameterInfo.Register);
}

void FD3D12CommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetUnorderedAccessParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    for (int32 Index = 0; Index < InUnorderedAccessViews.GetSize(); ++Index)
    {
        FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(InUnorderedAccessViews[Index]);
        State.DescriptorCache.SetUnorderedAccessView(D3D12Shader->GetShaderVisibility(), D3D12UnorderedAccessView, ParameterInfo.Register + Index);
    }
}

void FD3D12CommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetConstantBufferParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    if (ConstantBuffer)
    {
        FD3D12ConstantBufferView& D3D12ConstantBufferView = static_cast<FD3D12ConstantBuffer*>(ConstantBuffer)->GetView();
        State.DescriptorCache.SetConstantBufferView(D3D12Shader->GetShaderVisibility(), &D3D12ConstantBufferView, ParameterInfo.Register);
    }
    else
    {
        State.DescriptorCache.SetConstantBufferView(D3D12Shader->GetShaderVisibility(), nullptr, ParameterInfo.Register);
    }
}

void FD3D12CommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIConstantBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetConstantBufferParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    for (int32 Index = 0; Index < InConstantBuffers.GetSize(); ++Index)
    {
        if (InConstantBuffers[Index])
        {
            FD3D12ConstantBufferView& D3D12ConstantBufferView = static_cast<FD3D12ConstantBuffer*>(InConstantBuffers[Index])->GetView();
            State.DescriptorCache.SetConstantBufferView(D3D12Shader->GetShaderVisibility(), &D3D12ConstantBufferView, ParameterInfo.Register + Index);
        }
        else
        {
            State.DescriptorCache.SetConstantBufferView(D3D12Shader->GetShaderVisibility(), nullptr, ParameterInfo.Register + Index);
        }
    }
}

void FD3D12CommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetSamplerStateParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    FD3D12SamplerState* D3D12SamplerState = static_cast<FD3D12SamplerState*>(SamplerState);
    State.DescriptorCache.SetSamplerState(D3D12Shader->GetShaderVisibility(), D3D12SamplerState, ParameterInfo.Register);
}

void FD3D12CommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    FD3D12Shader* D3D12Shader = GetD3D12Shader(Shader);
    Check(D3D12Shader != nullptr);

    FD3D12ShaderParameter ParameterInfo = D3D12Shader->GetSamplerStateParameter(ParameterIndex);
    D3D12_ERROR_COND(ParameterInfo.Space == 0, "Global variables must be bound to RegisterSpace=0");

    for (int32 Index = 0; Index < InSamplerStates.GetSize(); ++Index)
    {
        FD3D12SamplerState* D3D12SamplerState = static_cast<FD3D12SamplerState*>(InSamplerStates[Index]);
        State.DescriptorCache.SetSamplerState(D3D12Shader->GetShaderVisibility(), D3D12SamplerState, ParameterInfo.Register + Index);
    }
}

void FD3D12CommandContext::ResolveTexture(FRHITexture* Destination, FRHITexture* Source)
{
    D3D12_ERROR_COND(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Texture* D3D12Destination = GetD3D12Texture(Destination);
    FD3D12Texture* D3D12Source      = GetD3D12Texture(Source);
    const DXGI_FORMAT DstFormat = D3D12Destination->GetDXGIFormat();
    const DXGI_FORMAT SrcFormat = D3D12Source->GetDXGIFormat();

    //TODO: For now texture must be the same format. I.e typeless does probably not work
    D3D12_ERROR_COND(DstFormat == SrcFormat, "Destination and Source texture must have the same format");

    CommandList->ResolveSubresource(D3D12Destination->GetResource(), D3D12Source->GetResource(), DstFormat);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void FD3D12CommandContext::UpdateBuffer(FRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    if (SizeInBytes > 0)
    {
        FD3D12Buffer* D3D12Destination = GetD3D12Buffer(Destination);
        UpdateBuffer(D3D12Destination->GetD3D12Resource(), OffsetInBytes, SizeInBytes, SourceData);

        CmdBatch->AddInUseResource(Destination);
    }
}

void FD3D12CommandContext::UpdateTexture2D(FRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
    D3D12_ERROR_COND(Destination != nullptr, "Destination cannot be nullptr");

    if (Width > 0 && Height > 0)
    {
        D3D12_ERROR_COND(SourceData != nullptr, "SourceData cannot be nullptr");

        FlushResourceBarriers();

        FD3D12Texture* D3D12Destination = GetD3D12Texture(Destination);

        const DXGI_FORMAT NativeFormat = D3D12Destination->GetDXGIFormat();
        
        const uint32 Stride      = GetFormatStride(NativeFormat);
        const uint32 RowPitch    = ((Width * Stride) + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
        const uint32 SizeInBytes = Height * RowPitch;

        FD3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();
        FD3D12UploadAllocation Allocation = GpuResourceUploader.LinearAllocate(SizeInBytes);

        const uint8* Source = reinterpret_cast<const uint8*>(SourceData);
        for (uint32 y = 0; y < Height; y++)
        {
            FMemory::Memcpy(Allocation.MappedPtr + (y * RowPitch), Source + (y * Width * Stride), Width * Stride);
        }

        // Copy to Dest
        D3D12_TEXTURE_COPY_LOCATION SourceLocation;
        FMemory::Memzero(&SourceLocation);

        SourceLocation.pResource                          = GpuResourceUploader.GetGpuResource();
        SourceLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        SourceLocation.PlacedFootprint.Footprint.Format   = NativeFormat;
        SourceLocation.PlacedFootprint.Footprint.Width    = Width;
        SourceLocation.PlacedFootprint.Footprint.Height   = Height;
        SourceLocation.PlacedFootprint.Footprint.Depth    = 1;
        SourceLocation.PlacedFootprint.Footprint.RowPitch = RowPitch;
        SourceLocation.PlacedFootprint.Offset             = Allocation.ResourceOffset;

        // TODO: MipLevel may not be the correct subresource
        D3D12_TEXTURE_COPY_LOCATION DestLocation;
        FMemory::Memzero(&DestLocation);

        DestLocation.pResource        = D3D12Destination->GetResource()->GetD3D12Resource();
        DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DestLocation.SubresourceIndex = MipLevel;

        CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);
        CmdBatch->AddInUseResource(Destination);
    }
}

void FD3D12CommandContext::CopyBuffer(FRHIBuffer* Destination, FRHIBuffer* Source, const FRHICopyBufferInfo& CopyInfo)
{
    D3D12_ERROR_COND(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Buffer* D3D12Destination = GetD3D12Buffer(Destination);
    Check(D3D12Destination != nullptr);

    FD3D12Buffer* D3D12Source = GetD3D12Buffer(Source);
    Check(D3D12Source != nullptr);

    CommandList->CopyBufferRegion(
        D3D12Destination->GetD3D12Resource(),
        CopyInfo.DestinationOffset,
        D3D12Source->GetD3D12Resource(),
        CopyInfo.SourceOffset,
        CopyInfo.SizeInBytes);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void FD3D12CommandContext::CopyTexture(FRHITexture* Destination, FRHITexture* Source)
{
    D3D12_ERROR_COND(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FlushResourceBarriers();

    FD3D12Texture* D3D12Destination = GetD3D12Texture(Destination);
    Check(D3D12Destination != nullptr);

    FD3D12Texture* D3D12Source = GetD3D12Texture(Source);
    Check(D3D12Source != nullptr);
    
    CommandList->CopyResource(D3D12Destination->GetResource(), D3D12Source->GetResource());

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void FD3D12CommandContext::CopyTextureRegion(FRHITexture* Destination, FRHITexture* Source, const FRHICopyTextureInfo& CopyInfo)
{
    D3D12_ERROR_COND(Destination != nullptr && Source != nullptr, "Destination or Source cannot be nullptr");

    FD3D12Texture* D3D12Destination = GetD3D12Texture(Destination);
    Check(D3D12Destination != nullptr);
    
    FD3D12Texture* D3D12Source = GetD3D12Texture(Source);
    Check(D3D12Source != nullptr);

    // Source
    D3D12_TEXTURE_COPY_LOCATION SourceLocation;
    FMemory::Memzero(&SourceLocation);

    SourceLocation.pResource        = D3D12Source->GetResource()->GetD3D12Resource();
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
    FMemory::Memzero(&DestinationLocation);

    DestinationLocation.pResource        = D3D12Destination->GetResource()->GetD3D12Resource();
    DestinationLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestinationLocation.SubresourceIndex = CopyInfo.Destination.SubresourceIndex;

    FlushResourceBarriers();

    CommandList->CopyTextureRegion(
        &DestinationLocation,
        CopyInfo.Destination.x,
        CopyInfo.Destination.y,
        CopyInfo.Destination.z,
        &SourceLocation, 
        &SourceBox);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void FD3D12CommandContext::DestroyResource(IRefCounted* Resource)
{
    CmdBatch->AddInUseResource(Resource);
}

void FD3D12CommandContext::DiscardContents(FRHITexture* Texture)
{
    // TODO: Enable regions to be discarded

    FD3D12Resource* D3D12Resource = GetD3D12Resource(Texture);
    if (D3D12Resource)
    {
        CommandList->DiscardResource(D3D12Resource->GetD3D12Resource(), nullptr);
        CmdBatch->AddInUseResource(Texture);
    }
}

void FD3D12CommandContext::BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)
{
    D3D12_ERROR_COND(RayTracingGeometry != nullptr, "RayTracingGeometry cannot be nullptr");

    FlushResourceBarriers();

    FD3D12VertexBuffer* D3D12VertexBuffer = static_cast<FD3D12VertexBuffer*>(VertexBuffer);
    FD3D12IndexBuffer*  D3D12IndexBuffer  = static_cast<FD3D12IndexBuffer*>(IndexBuffer);
    D3D12_ERROR_COND(D3D12VertexBuffer != nullptr, "VertexBuffer cannot be nullptr");

    FD3D12RayTracingGeometry* D3D12Geometry = static_cast<FD3D12RayTracingGeometry*>(RayTracingGeometry);
    D3D12Geometry->Build(*this, D3D12VertexBuffer, D3D12IndexBuffer, bUpdate);

    CmdBatch->AddInUseResource(RayTracingGeometry);
    CmdBatch->AddInUseResource(VertexBuffer);
    CmdBatch->AddInUseResource(IndexBuffer);
}

void FD3D12CommandContext::BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
    D3D12_ERROR_COND(RayTracingScene != nullptr, "RayTracingScene cannot be nullptr");

    FlushResourceBarriers();

    FD3D12RayTracingScene* D3D12Scene = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    D3D12Scene->Build(*this, Instances, bUpdate);

    CmdBatch->AddInUseResource(RayTracingScene);
}

void FD3D12CommandContext::SetRayTracingBindings(
    FRHIRayTracingScene* RayTracingScene,
    FRHIRayTracingPipelineState* PipelineState,
    const FRayTracingShaderResources* GlobalResource,
    const FRayTracingShaderResources* RayGenLocalResources,
    const FRayTracingShaderResources* MissLocalResources,
    const FRayTracingShaderResources* HitGroupResources,
    uint32 NumHitGroupResources)
{
    FD3D12RayTracingScene*         D3D12Scene         = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    FD3D12RayTracingPipelineState* D3D12PipelineState = static_cast<FD3D12RayTracingPipelineState*>(PipelineState);
    D3D12_ERROR_COND(D3D12Scene         != nullptr, "RayTracingScene cannot be nullptr");
    D3D12_ERROR_COND(D3D12PipelineState != nullptr, "PipelineState cannot be nullptr");

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

    D3D12_ERROR_COND(
        NumDescriptorsNeeded < D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT,
        "NumDescriptorsNeeded=%u, but the maximum is '%u'",
        NumDescriptorsNeeded,
        D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT);

    FD3D12OnlineDescriptorManager* ResourceHeap = CmdBatch->GetResourceDescriptorManager();
    if (!ResourceHeap->HasSpace(NumDescriptorsNeeded))
    {
        Check(false);
        // TODO: Fix this
        // ResourceHeap->AllocateFreshHeap();
    }

    D3D12_ERROR_COND(
        NumSamplersNeeded < D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT,
        "NumDescriptorsNeeded=%u, but the maximum is '%u'",
        NumSamplersNeeded,
        D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT);

    FD3D12OnlineDescriptorManager* SamplerHeap = CmdBatch->GetSamplerDescriptorManager();
    if (!SamplerHeap->HasSpace(NumSamplersNeeded))
    {
        Check(false);
        // TODO: Fix this
        // SamplerHeap->AllocateFreshHeap();
    }

    // TODO: Fix this
    // if (!D3D12Scene->BuildBindingTable(*this, D3D12PipelineState, ResourceHeap, SamplerHeap, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources))
    {
        D3D12_ERROR("[FD3D12CommandContext]: FAILED to Build Shader Binding Table");
    }

    if (GlobalResource)
    {
        if (!GlobalResource->ConstantBuffers.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ConstantBuffers.GetSize(); i++)
            {
                FD3D12ConstantBufferView& D3D12ConstantBufferView = static_cast<FD3D12ConstantBuffer*>(GlobalResource->ConstantBuffers[i])->GetView();
                State.DescriptorCache.SetConstantBufferView(ShaderVisibility_All, &D3D12ConstantBufferView, i);
            }
        }
        if (!GlobalResource->ShaderResourceViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->ShaderResourceViews.GetSize(); i++)
            {
                FD3D12ShaderResourceView* D3D12ShaderResourceView = static_cast<FD3D12ShaderResourceView*>(GlobalResource->ShaderResourceViews[i]);
                State.DescriptorCache.SetShaderResourceView(ShaderVisibility_All, D3D12ShaderResourceView, i);
            }
        }
        if (!GlobalResource->UnorderedAccessViews.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->UnorderedAccessViews.GetSize(); i++)
            {
                FD3D12UnorderedAccessView* D3D12UnorderedAccessView = static_cast<FD3D12UnorderedAccessView*>(GlobalResource->UnorderedAccessViews[i]);
                State.DescriptorCache.SetUnorderedAccessView(ShaderVisibility_All, D3D12UnorderedAccessView, i);
            }
        }
        if (!GlobalResource->SamplerStates.IsEmpty())
        {
            for (int32 i = 0; i < GlobalResource->SamplerStates.GetSize(); i++)
            {
                FD3D12SamplerState* DxSampler = static_cast<FD3D12SamplerState*>(GlobalResource->SamplerStates[i]);
                State.DescriptorCache.SetSamplerState(ShaderVisibility_All, DxSampler, i);
            }
        }
    }

    ID3D12GraphicsCommandList4* DXRCommandList = CommandList->GetGraphicsCommandList4();

    FD3D12RootSignature* GlobalRootSignature = D3D12PipelineState->GetGlobalRootSignature();
    DXRCommandList->SetComputeRootSignature(GlobalRootSignature->GetD3D12RootSignature());

    State.DescriptorCache.PrepareComputeDescriptors(CmdBatch, GlobalRootSignature);
}

void FD3D12CommandContext::GenerateMips(FRHITexture* Texture)
{
    FD3D12Texture* D3D12Texture = GetD3D12Texture(Texture);
    D3D12_ERROR_COND(D3D12Texture != nullptr, "Texture cannot be nullptr");

    D3D12_RESOURCE_DESC Desc = D3D12Texture->GetResource()->GetDesc();
    Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_ERROR_COND(Desc.MipLevels > 1, "MipLevels must be more than one in order to generate any MipLevels");

    // TODO: Create this placed from a Heap? See what performance is 
    FD3D12ResourceRef StagingTexture = dbg_new FD3D12Resource(GetDevice(), Desc, D3D12Texture->GetResource()->GetHeapType());
    if (!StagingTexture->Initialize(D3D12_RESOURCE_STATE_COMMON, nullptr))
    {
        LOG_ERROR("[FD3D12CommandContext] Failed to create StagingTexture for GenerateMips");
        return;
    }
    else
    {
        StagingTexture->SetName("GenerateMips StagingTexture");
    }

    // Check Type
    const bool bIsTextureCube = (Texture->GetTextureCube() != nullptr);

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
    FMemory::Memzero(&SrvDesc);

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
    FMemory::Memzero(&UavDesc);

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

    FD3D12OnlineDescriptorManager* ResourceDescriptors = CmdBatch->GetResourceDescriptorManager();

    // Allocate an extra handle for SRV
    const uint32 StartDescriptorHandleIndex = ResourceDescriptors->AllocateHandles(UavDescriptorHandleCount + 1);

    const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle_CPU = ResourceDescriptors->GetCPUDescriptorHandleAt(StartDescriptorHandleIndex);
    GetDevice()->GetD3D12Device()->CreateShaderResourceView(
        D3D12Texture->GetResource()->GetD3D12Resource(), 
        &SrvDesc, 
        SrvHandle_CPU);

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

        const D3D12_CPU_DESCRIPTOR_HANDLE UavHandle_CPU = ResourceDescriptors->GetCPUDescriptorHandleAt(UavStartDescriptorHandleIndex + i);
        GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(
            StagingTexture->GetD3D12Resource(), 
            nullptr, 
            &UavDesc, 
            UavHandle_CPU);
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

        const D3D12_CPU_DESCRIPTOR_HANDLE UavHandle_CPU = ResourceDescriptors->GetCPUDescriptorHandleAt(UavStartDescriptorHandleIndex + i);
        GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(nullptr, nullptr, &UavDesc, UavHandle_CPU);
    }

    // We assume the destination is in D3D12_RESOURCE_STATE_COPY_DEST
    TransitionResource(D3D12Texture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    
    FlushResourceBarriers();

    CommandList->CopyResource(StagingTexture.Get(), D3D12Texture->GetResource());

    TransitionResource(D3D12Texture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    
    FlushResourceBarriers();

    FD3D12Interface* D3D12CoreInterface = GetDevice()->GetAdapter()->GetD3D12Interface();
    if (bIsTextureCube)
    {
        FD3D12ComputePipelineStateRef PipelineState = D3D12CoreInterface->GetGenerateMipsPipelineTexureCube();
        CommandList->SetPipelineState(PipelineState->GetD3D12PipelineState());
        CommandList->SetComputeRootSignature(PipelineState->GetRootSignature());
    }
    else
    {
        FD3D12ComputePipelineStateRef PipelineState = D3D12CoreInterface->GetGenerateMipsPipelineTexure2D();
        CommandList->SetPipelineState(PipelineState->GetD3D12PipelineState());
        CommandList->SetComputeRootSignature(PipelineState->GetRootSignature());
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE SrvHandle_GPU = ResourceDescriptors->GetGPUDescriptorHandleAt(StartDescriptorHandleIndex);

    ID3D12DescriptorHeap* OnlineResourceHeap = ResourceDescriptors->GetHeap()->GetD3D12Heap();
    CommandList->SetDescriptorHeaps(&OnlineResourceHeap, 1);

    struct FConstantBuffer
    {
        uint32   SrcMipLevel;
        uint32   NumMipLevels;
        FVector2 TexelSize;
    } ConstantData;

    uint32 DstWidth  = static_cast<uint32>(Desc.Width);
    uint32 DstHeight = Desc.Height;
    ConstantData.SrcMipLevel = 0;

    const uint32 ThreadsZ = bIsTextureCube ? 6 : 1;

    uint32 RemainingMiplevels = Desc.MipLevels;
    for (uint32 i = 0; i < NumDispatches; i++)
    {
        ConstantData.TexelSize    = FVector2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));
        ConstantData.NumMipLevels = NMath::Min<uint32>(4, RemainingMiplevels);

        CommandList->SetComputeRoot32BitConstants(&ConstantData, 4, 0, 0);
        CommandList->SetComputeRootDescriptorTable(SrvHandle_GPU, 1);

        const uint32 GPUDescriptorHandleIndex = i * MipLevelsPerDispatch;

        const D3D12_GPU_DESCRIPTOR_HANDLE UavHandle_GPU = ResourceDescriptors->GetGPUDescriptorHandleAt(UavStartDescriptorHandleIndex + GPUDescriptorHandleIndex);
        CommandList->SetComputeRootDescriptorTable(UavHandle_GPU, 2);

        constexpr uint32 ThreadCount = 8;

        const uint32 ThreadsX = NMath::DivideByMultiple(DstWidth, ThreadCount);
        const uint32 ThreadsY = NMath::DivideByMultiple(DstHeight, ThreadCount);
        CommandList->Dispatch(ThreadsX, ThreadsY, ThreadsZ);

        UnorderedAccessBarrier(StagingTexture.Get());

        TransitionResource(D3D12Texture->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();

        // TODO: Copy only MipLevels (Maybe faster?)
        CommandList->CopyResource(D3D12Texture->GetResource(), StagingTexture.Get());

        TransitionResource(D3D12Texture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(StagingTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        FlushResourceBarriers();

        DstWidth  = DstWidth / 16;
        DstHeight = DstHeight / 16;

        ConstantData.SrcMipLevel += 3;
        RemainingMiplevels       -= MipLevelsPerDispatch;
    }

    TransitionResource(D3D12Texture->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

    FlushResourceBarriers();

    CmdBatch->AddInUseResource(StagingTexture.Get());
}

void FD3D12CommandContext::TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    FD3D12Texture* D3D12Texture = GetD3D12Texture(Texture);
    TransitionResource(D3D12Texture->GetResource(), D3D12BeforeState, D3D12AfterState);

    CmdBatch->AddInUseResource(Texture);
}

void FD3D12CommandContext::TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    const D3D12_RESOURCE_STATES D3D12BeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES D3D12AfterState  = ConvertResourceState(AfterState);

    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Buffer);
    Check(D3D12Buffer != nullptr);

    TransitionResource(D3D12Buffer->GetD3D12Resource(), D3D12BeforeState, D3D12AfterState);

    CmdBatch->AddInUseResource(Buffer);
}

void FD3D12CommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    FD3D12Texture* D3D12Texture = GetD3D12Texture(Texture);
    Check(D3D12Texture != nullptr);

    UnorderedAccessBarrier(D3D12Texture->GetResource());

    CmdBatch->AddInUseResource(Texture);
}

void FD3D12CommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Buffer);
    Check(D3D12Buffer != nullptr);

    UnorderedAccessBarrier(D3D12Buffer->GetD3D12Resource());

    CmdBatch->AddInUseResource(Buffer);
}

void FD3D12CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    FlushResourceBarriers();

    State.ApplyGraphics(*CommandList, CmdBatch);
    CommandList->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void FD3D12CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    FlushResourceBarriers();

    State.ApplyGraphics(*CommandList, CmdBatch);
    CommandList->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FD3D12CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    State.ApplyGraphics(*CommandList, CmdBatch);
    CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    State.ApplyGraphics(*CommandList, CmdBatch);
    CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FD3D12CommandContext::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    FlushResourceBarriers();

    State.ApplyCompute(*CommandList, CmdBatch);
    CommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FD3D12CommandContext::DispatchRays(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, uint32 Width, uint32 Height, uint32 Depth)
{
    FD3D12RayTracingScene* D3D12Scene = static_cast<FD3D12RayTracingScene*>(RayTracingScene);
    D3D12_ERROR_COND(D3D12Scene != nullptr, "RayTracingScene cannot be nullptr");

    FD3D12RayTracingPipelineState* D3D12PipelineState = static_cast<FD3D12RayTracingPipelineState*>(PipelineState);
    D3D12_ERROR_COND(D3D12PipelineState != nullptr, "PipelineState cannot be nullptr");

    ID3D12GraphicsCommandList4* DXRCommandList = CommandList->GetGraphicsCommandList4();
    D3D12_ERROR_COND(DXRCommandList != nullptr, "DXRCommandList is nullptr, DXR is not supported");

    FlushResourceBarriers();

    D3D12_DISPATCH_RAYS_DESC RayDispatchDesc;
    FMemory::Memzero(&RayDispatchDesc);

    RayDispatchDesc.RayGenerationShaderRecord = D3D12Scene->GetRayGenShaderRecord();
    RayDispatchDesc.MissShaderTable           = D3D12Scene->GetMissShaderTable();
    RayDispatchDesc.HitGroupTable             = D3D12Scene->GetHitGroupTable();

    RayDispatchDesc.Width  = Width;
    RayDispatchDesc.Height = Height;
    RayDispatchDesc.Depth  = Depth;

    DXRCommandList->SetPipelineState1(D3D12PipelineState->GetD3D12StateObject());
    DXRCommandList->DispatchRays(&RayDispatchDesc);
}

void FD3D12CommandContext::PresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    // Ensure that commands are submitted
    EndCommandList();

    FD3D12Viewport* D3D12Viewport = static_cast<FD3D12Viewport*>(Viewport);
    D3D12Viewport->Present(bVerticalSync);

    // Start recording again
    AquireCommandList();
}

void FD3D12CommandContext::ClearState()
{
    Flush();

    InternalClearState();
}

void FD3D12CommandContext::Flush()
{
    TScopedLock Lock(CommandContextCS);

    if (State.bIsReady)
    {
        EndCommandList();
    }

    const uint64 NewFenceValue = ++FenceValue;

    FD3D12CommandListManager* CommandListManager = GetDevice()->GetCommandListManager(QueueType);
    HRESULT hResult = CommandListManager->GetD3D12CommandQueue()->Signal(Fence.GetFence(), NewFenceValue);
    if (SUCCEEDED(hResult))
    {
        Fence.WaitForValue(NewFenceValue);

        for (FD3D12CommandBatch& Batch : CmdBatches)
        {
            Batch.Reset();
        }
    }
}

void FD3D12CommandContext::InsertMarker(const FStringView& Message)
{
    if (FDynamicD3D12::SetMarkerOnCommandList)
    {
        FDynamicD3D12::SetMarkerOnCommandList(CommandList->GetGraphicsCommandList(), PIX_COLOR(255, 255, 255), Message.GetCString());
    }
}

void FD3D12CommandContext::BeginExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetAdapter()->GetGraphicsAnalysis();
    if (GraphicsAnalysis && !State.bIsCapturing)
    {
        GraphicsAnalysis->BeginCapture();
        State.bIsCapturing = true;
    }
}

void FD3D12CommandContext::EndExternalCapture()
{
    IDXGraphicsAnalysis* GraphicsAnalysis = GetDevice()->GetAdapter()->GetGraphicsAnalysis();
    if (GraphicsAnalysis && State.bIsCapturing)
    {
        GraphicsAnalysis->EndCapture();
        State.bIsCapturing = false;
    }
}

void FD3D12CommandContext::InternalClearState()
{
    State.ClearAll();
}

void FD3D12CommandContext::AquireCommandList()
{
    Check(State.bIsReady == false);

    TRACE_FUNCTION_SCOPE();

    CmdBatch = &CmdBatches[NextCmdBatch];
    NextCmdBatch = (NextCmdBatch + 1) % CmdBatches.GetSize();

    if (CmdBatch->AssignedFenceValue >= Fence.GetCompletedValue())
    {
        Fence.WaitForValue(CmdBatch->AssignedFenceValue);
    }

    if (!CmdBatch->Reset())
    {
        D3D12_ERROR("Failed to reset D3D12CommandBatch");
        return;
    }

    InternalClearState();

    if (!CommandList->Reset(CmdBatch->GetCommandAllocator()))
    {
        D3D12_ERROR("Failed to reset Commandlist");
    }

    State.DescriptorCache.SetCurrentCommandList(CommandList.Get());
    State.bIsReady = true;
}

void FD3D12CommandContext::EndCommandList()
{
    Check(State.bIsReady == true);

    TRACE_FUNCTION_SCOPE();

    FlushResourceBarriers();

    const uint64 NewFenceValue = ++FenceValue;
    CmdBatch->AssignedFenceValue = NewFenceValue;
    CmdBatch = nullptr;

    for (int32 QueryIndex = 0; QueryIndex < ResolveQueries.GetSize(); ++QueryIndex)
    {
        ResolveQueries[QueryIndex]->ResolveQueries(*this);
    }

    ResolveQueries.Clear();

    // Execute
    if (!CommandList->Close())
    {
        D3D12_ERROR("Failed to close CommandList");
        return;
    }

    FD3D12CommandListManager* CommandListManager = GetDevice()->GetCommandListManager(QueueType);
    CommandListManager->ExecuteCommandList(CommandList);

    HRESULT hResult = CommandListManager->GetD3D12CommandQueue()->Signal(Fence.GetFence(), NewFenceValue);
    if (FAILED(hResult))
    {
        D3D12_ERROR("Failed to signal Fence on the GPU");
    }

    // Clear the state
    State.ClearAll();
    State.DescriptorCache.SetCurrentCommandList(nullptr);
}
