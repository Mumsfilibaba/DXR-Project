#include "D3D12CommandContext.h"
#include "D3D12Device.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandList.h"
#include "D3D12Helpers.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12PipelineState.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Shader.h"
#include "D3D12RayTracing.h"
#include "D3D12RenderLayer.h"
#include "D3D12GPUProfiler.h"

#include <pix.h>

void D3D12ResourceBarrierBatcher::AddTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
    Assert(Resource != nullptr);

    if (BeforeState != AfterState)
    {
        // Make sure we are not already have transition for this resource
        for (TArray<D3D12_RESOURCE_BARRIER>::Iterator It = Barriers.Begin(); It != Barriers.End(); It++)
        {
            if ((*It).Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
            {
                if ((*It).Transition.pResource == Resource)
                {
                    if ((*It).Transition.StateBefore == AfterState)
                    {
                        It = Barriers.Erase(It);
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
        Memory::Memzero(&Barrier);

        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = Resource;
        Barrier.Transition.StateAfter  = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        Barriers.EmplaceBack(Barrier);
    }
}

D3D12GPUResourceUploader::D3D12GPUResourceUploader(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
    , MappedMemory(nullptr)
    , SizeInBytes(0)
    , OffsetInBytes(0)
    , Resource(nullptr)
    , GarbageResources()
{
}

bool D3D12GPUResourceUploader::Reserve(uint32 InSizeInBytes)
{
    if (InSizeInBytes == SizeInBytes)
    {
        return true;
    }

    if (Resource)
    {
        Resource->Unmap(0, nullptr);
        GarbageResources.EmplaceBack(Resource);
        Resource.Reset();
    }

    D3D12_HEAP_PROPERTIES HeapProperties;
    Memory::Memzero(&HeapProperties);

    HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC Desc;
    Memory::Memzero(&Desc);

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

void D3D12GPUResourceUploader::Reset()
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

D3D12UploadAllocation D3D12GPUResourceUploader::LinearAllocate(uint32 InSizeInBytes)
{
    constexpr uint32 EXTRA_BYTES_ALLOCATED = 1024;

    const uint32 NeededSize = OffsetInBytes + InSizeInBytes;
    if (NeededSize > SizeInBytes)
    {
        Reserve(NeededSize + EXTRA_BYTES_ALLOCATED);
    }

    D3D12UploadAllocation Allocation;
    Allocation.MappedPtr      = MappedMemory + OffsetInBytes;
    Allocation.ResourceOffset = OffsetInBytes;
    OffsetInBytes += InSizeInBytes;
    return Allocation;
}

D3D12CommandBatch::D3D12CommandBatch(D3D12Device* InDevice)
    : Device(InDevice)
    , CmdAllocator(InDevice)
    , GpuResourceUploader(InDevice)
    , OnlineResourceDescriptorHeap(nullptr)
    , OnlineSamplerDescriptorHeap(nullptr)
    , Resources()
{
}

bool D3D12CommandBatch::Init()
{
    // TODO: Do not have D3D12_COMMAND_LIST_TYPE_DIRECT directly
    if (!CmdAllocator.Init(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    OnlineResourceDescriptorHeap = DBG_NEW D3D12OnlineDescriptorHeap(
        Device,
        D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_COUNT,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!OnlineResourceDescriptorHeap->Init())
    {
        return false;
    }

    OnlineSamplerDescriptorHeap = DBG_NEW D3D12OnlineDescriptorHeap(Device,
        D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_COUNT,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!OnlineSamplerDescriptorHeap->Init())
    {
        return false;
    }

    OnlineRayTracingResourceDescriptorHeap = DBG_NEW D3D12OnlineDescriptorHeap(
        Device,
        D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_COUNT,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!OnlineRayTracingResourceDescriptorHeap->Init())
    {
        return false;
    }

    OnlineRayTracingSamplerDescriptorHeap = DBG_NEW D3D12OnlineDescriptorHeap(Device,
        D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_COUNT,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!OnlineRayTracingSamplerDescriptorHeap->Init())
    {
        return false;
    }

    GpuResourceUploader.Reserve(1024);
    return true;
}

D3D12CommandContext::D3D12CommandContext(D3D12Device* InDevice)
    : ICommandContext()
    , D3D12DeviceChild(InDevice)
    , CmdQueue(InDevice)
    , CmdList(InDevice)
    , Fence(InDevice)
    , DescriptorCache(InDevice)
    , CmdBatches()
    , BarrierBatcher()
{
}

D3D12CommandContext::~D3D12CommandContext()
{
    Flush();
}

bool D3D12CommandContext::Init()
{
    if (!CmdQueue.Init(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    // TODO: Have support for more than 3 commandbatches?
    for (uint32 i = 0; i < 3; i++)
    {
        D3D12CommandBatch& Batch = CmdBatches.EmplaceBack(GetDevice());
        if (!Batch.Init())
        {
            return false;
        }
    }

    if (!CmdList.Init(D3D12_COMMAND_LIST_TYPE_DIRECT, CmdBatches[0].GetCommandAllocator(), nullptr))
    {
        return false;
    }

    FenceValue = 0;
    if (!Fence.Init(FenceValue))
    {
        return false;
    }

    if (!DescriptorCache.Init())
    {
        return false;
    }

    TArray<uint8> Code;
    if (!gD3D12ShaderCompiler->CompileFromFile("../DXR-Engine/Shaders/GenerateMipsTex2D.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTex2D Shader");
        
        Debug::DebugBreak();
        return false;
    }

    TRef<D3D12ComputeShader> Shader = DBG_NEW D3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        Debug::DebugBreak();
        return false;
    }

    GenerateMipsTex2D_PSO = DBG_NEW D3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTex2D_PSO->Init())
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTex2D PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTex2D_PSO->SetName("GenerateMipsTex2D Gen PSO");
    }

    if (!gD3D12ShaderCompiler->CompileFromFile("../DXR-Engine/Shaders/GenerateMipsTexCube.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTexCube Shader");
        Debug::DebugBreak();
    }

    Shader = DBG_NEW D3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        Debug::DebugBreak();
        return false;
    }

    GenerateMipsTexCube_PSO = DBG_NEW D3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTexCube_PSO->Init())
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTexCube PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTexCube_PSO->SetName("GenerateMipsTexCube Gen PSO");
    }

    return true;
}

void D3D12CommandContext::UpdateBuffer(D3D12Resource* Resource, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    if (SizeInBytes != 0)
    {
        FlushResourceBarriers();

        D3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();

        D3D12UploadAllocation Allocation = GpuResourceUploader.LinearAllocate((uint32)SizeInBytes);
        Memory::Memcpy(Allocation.MappedPtr, SourceData, SizeInBytes);

        CmdList.CopyBufferRegion(Resource->GetResource(), OffsetInBytes, GpuResourceUploader.GetGpuResource(), Allocation.ResourceOffset, SizeInBytes);

        CmdBatch->AddInUseResource(Resource);
    }
}

void D3D12CommandContext::Begin()
{
    Assert(IsReady == false);

    CmdBatch     = &CmdBatches[NextCmdBatch];
    NextCmdBatch = (NextCmdBatch + 1) % CmdBatches.Size();


    // TODO: Investigate better ways of doing this 
    if (FenceValue >= CmdBatches.Size())
    {
        const uint64 WaitValue = Math::Max<uint64>(FenceValue - (CmdBatches.Size() - 1), 0);
        Fence.WaitForValue(WaitValue);
    }

    if (!CmdBatch->Reset())
    {
        Debug::DebugBreak();
        return;
    }

    InternalClearState();

    if (!CmdList.Reset(CmdBatch->GetCommandAllocator()))
    {
        Debug::DebugBreak();
        return;
    }

    IsReady = true;
}

void D3D12CommandContext::End()
{
    Assert(IsReady == true);

    FlushResourceBarriers();

    CmdBatch = nullptr;
    IsReady  = false;

    const uint64 NewFenceValue = ++FenceValue;
    for (uint32 i = 0; i < ResolveProfilers.Size(); i++)
    {
        ResolveProfilers[i]->ResolveQueries(*this);
    }
    ResolveProfilers.Clear();

    // Execute
    if (!CmdList.Close())
    {
        Debug::DebugBreak();
        return;
    }

    CmdQueue.ExecuteCommandList(&CmdList);

    if (!CmdQueue.SignalFence(Fence, NewFenceValue))
    {
        Debug::DebugBreak();
        return;
    }
}

void D3D12CommandContext::BeginTimeStamp(GPUProfiler* Profiler, uint32 Index)
{
    ID3D12GraphicsCommandList* DxCmdList  = CmdList.GetGraphicsCommandList();
    
    D3D12GPUProfiler* DxProfiler = static_cast<D3D12GPUProfiler*>(Profiler);
    Assert(Profiler);
    
    DxProfiler->BeginQuery(DxCmdList, Index);

    ResolveProfilers.EmplaceBack(MakeSharedRef<D3D12GPUProfiler>(DxProfiler));
}

void D3D12CommandContext::EndTimeStamp(GPUProfiler* Profiler, uint32 Index)
{
    Assert(Profiler);

    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

    D3D12GPUProfiler* DxProfiler = static_cast<D3D12GPUProfiler*>(Profiler);
    Assert(Profiler);

    DxProfiler->EndQuery(DxCmdList, Index);
}

void D3D12CommandContext::ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorF& ClearColor)
{
    Assert(RenderTargetView != nullptr);

    FlushResourceBarriers();

    D3D12RenderTargetView* DxRenderTargetView = static_cast<D3D12RenderTargetView*>(RenderTargetView);
    CmdBatch->AddInUseResource(DxRenderTargetView);

    CmdList.ClearRenderTargetView(DxRenderTargetView->GetOfflineHandle(), ClearColor.Elements, 0, nullptr);
}

void D3D12CommandContext::ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilF& ClearValue) 
{
    Assert(DepthStencilView != nullptr);

    FlushResourceBarriers();

    D3D12DepthStencilView* DxDepthStencilView = static_cast<D3D12DepthStencilView*>(DepthStencilView);
    CmdBatch->AddInUseResource(DxDepthStencilView);
    
    CmdList.ClearDepthStencilView(DxDepthStencilView->GetOfflineHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, ClearValue.Depth, ClearValue.Stencil);
}

void D3D12CommandContext::ClearUnorderedAccessViewFloat(UnorderedAccessView* UnorderedAccessView, const ColorF& ClearColor)
{
    FlushResourceBarriers();
    
    D3D12UnorderedAccessView* DxUnorderedAccessView = static_cast<D3D12UnorderedAccessView*>(UnorderedAccessView);
    CmdBatch->AddInUseResource(DxUnorderedAccessView);

    D3D12OnlineDescriptorHeap* OnlineDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    const uint32 OnlineDescriptorHandleIndex = OnlineDescriptorHeap->AllocateHandles(1);

    const D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle    = DxUnorderedAccessView->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle_CPU = OnlineDescriptorHeap->GetCPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    GetDevice()->CopyDescriptorsSimple(1, OnlineHandle_CPU, OfflineHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandle_GPU = OnlineDescriptorHeap->GetGPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    CmdList.ClearUnorderedAccessViewFloat(OnlineHandle_GPU, DxUnorderedAccessView, ClearColor.Elements);
}

void D3D12CommandContext::SetShadingRate(EShadingRate ShadingRate)
{
    D3D12_SHADING_RATE DxShadingRate = ConvertShadingRate(ShadingRate);

    D3D12_SHADING_RATE_COMBINER Combiners[] =
    {
        D3D12_SHADING_RATE_COMBINER_OVERRIDE,
        D3D12_SHADING_RATE_COMBINER_OVERRIDE,
    };

    CmdList.RSSetShadingRate(DxShadingRate, Combiners);
}

void D3D12CommandContext::SetShadingRateImage(Texture2D* ShadingImage)
{
    FlushResourceBarriers();
    
    if (ShadingImage)
    {
        D3D12BaseTexture* DxTexture = D3D12TextureCast(ShadingImage);
        CmdList.RSSetShadingRateImage(DxTexture->GetResource()->GetResource());
        CmdBatch->AddInUseResource(ShadingImage);
    }
    else
    {
        CmdList.RSSetShadingRateImage(nullptr);
    }
}

void D3D12CommandContext::BeginRenderPass()
{
    // Empty for now
}

void D3D12CommandContext::EndRenderPass()
{
    // Empty for now
}

void D3D12CommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
{
    D3D12_VIEWPORT Viewport;
    Viewport.Width    = Width;
    Viewport.Height   = Height;
    Viewport.MaxDepth = MaxDepth;
    Viewport.MinDepth = MinDepth;
    Viewport.TopLeftX = x;
    Viewport.TopLeftY = y;

    CmdList.RSSetViewports(&Viewport, 1);
}

void D3D12CommandContext::SetScissorRect(float Width, float Height, float x, float y)
{
    D3D12_RECT ScissorRect;
    ScissorRect.top    = LONG(y);
    ScissorRect.bottom = LONG(Height);
    ScissorRect.left   = LONG(x);
    ScissorRect.right  = LONG(Width);

    CmdList.RSSetScissorRects(&ScissorRect, 1);
}

void D3D12CommandContext::SetBlendFactor(const ColorF& Color)
{
    CmdList.OMSetBlendFactor(Color.Elements);
}

void D3D12CommandContext::SetPrimitiveTopology(EPrimitiveTopology InPrimitveTopology)
{
    const D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = ConvertPrimitiveTopology(InPrimitveTopology);
    CmdList.IASetPrimitiveTopology(PrimitiveTopology);
}

void D3D12CommandContext::SetVertexBuffers(VertexBuffer* const * VertexBuffers, uint32 BufferCount, uint32 BufferSlot)
{
    Assert(BufferSlot + BufferCount < D3D12_MAX_VERTEX_BUFFER_SLOTS);

    for (uint32 i = 0; i < BufferCount; i++)
    {
        uint32 Slot = BufferSlot + i;
        D3D12VertexBuffer* DxVertexBuffer = static_cast<D3D12VertexBuffer*>(VertexBuffers[i]);
        DescriptorCache.SetVertexBuffer(DxVertexBuffer, Slot);
        
        // TODO: The DescriptorCache maybe should have this responsibility?
        CmdBatch->AddInUseResource(DxVertexBuffer);
    }
}

void D3D12CommandContext::SetIndexBuffer(IndexBuffer* IndexBuffer)
{
    D3D12IndexBuffer* DxIndexBuffer = static_cast<D3D12IndexBuffer*>(IndexBuffer);
    DescriptorCache.SetIndexBuffer(DxIndexBuffer);

    // TODO: Maybe this should be done by the descriptorcache
    CmdBatch->AddInUseResource(DxIndexBuffer);
}

void D3D12CommandContext::SetRenderTargets(RenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, DepthStencilView* DepthStencilView)
{
    for (uint32 i = 0; i < RenderTargetCount; i++)
    {
        D3D12RenderTargetView* DxRenderTargetView = static_cast<D3D12RenderTargetView*>(RenderTargetViews[i]);
        DescriptorCache.SetRenderTargetView(DxRenderTargetView, i);

        // TODO: Maybe this should be handled by the descriptorcache
        CmdBatch->AddInUseResource(DxRenderTargetView);
    }

    D3D12DepthStencilView* DxDepthStencilView = static_cast<D3D12DepthStencilView*>(DepthStencilView);
    DescriptorCache.SetDepthStencilView(DxDepthStencilView);
    CmdBatch->AddInUseResource(DxDepthStencilView);
}

void D3D12CommandContext::SetGraphicsPipelineState(class GraphicsPipelineState* PipelineState)
{
    Assert(PipelineState != nullptr);

    D3D12GraphicsPipelineState* DxPipelineState = static_cast<D3D12GraphicsPipelineState*>(PipelineState);
    if (DxPipelineState != CurrentGraphicsPipelineState.Get())
    {
        CurrentGraphicsPipelineState = MakeSharedRef<D3D12GraphicsPipelineState>(DxPipelineState);
        CmdList.SetPipelineState(CurrentGraphicsPipelineState->GetPipeline());
    }

    D3D12RootSignature* DxRootSignature = DxPipelineState->GetRootSignature();
    if (DxRootSignature != CurrentRootSignature.Get())
    {
        CurrentRootSignature = MakeSharedRef<D3D12RootSignature>(DxRootSignature);
        CmdList.SetGraphicsRootSignature(CurrentRootSignature.Get());
    }
}

void D3D12CommandContext::SetComputePipelineState(class ComputePipelineState* PipelineState)
{
    Assert(PipelineState != nullptr);

    D3D12ComputePipelineState* DxPipelineState = static_cast<D3D12ComputePipelineState*>(PipelineState);
    if (DxPipelineState != CurrentComputePipelineState.Get())
    {
        CurrentComputePipelineState = MakeSharedRef<D3D12ComputePipelineState>(DxPipelineState);
        CmdList.SetPipelineState(CurrentComputePipelineState->GetPipeline());
    }

    D3D12RootSignature* DxRootSignature = DxPipelineState->GetRootSignature();
    if (DxRootSignature != CurrentRootSignature.Get())
    {
        CurrentRootSignature = MakeSharedRef<D3D12RootSignature>(DxRootSignature);
        CmdList.SetComputeRootSignature(CurrentRootSignature.Get());
    }
}

void D3D12CommandContext::Set32BitShaderConstants(Shader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
    UNREFERENCED_VARIABLE(Shader);

    Assert(Num32BitConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT);
    ShaderConstantsCache.Set32BitShaderConstants((uint32*)Shader32BitConstants, Num32BitConstants);
}

void D3D12CommandContext::SetShaderResourceView(Shader* Shader, ShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetShaderResourceParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == 1);

    D3D12ShaderResourceView* DxShaderResourceView = static_cast<D3D12ShaderResourceView*>(ShaderResourceView);
    DescriptorCache.SetShaderResourceView(DxShaderResourceView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
}

void D3D12CommandContext::SetShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetShaderResourceParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == NumShaderResourceViews);

    for (uint32 i = 0; i < NumShaderResourceViews; i++)
    {
        D3D12ShaderResourceView* DxShaderResourceView = static_cast<D3D12ShaderResourceView*>(ShaderResourceView[i]);
        DescriptorCache.SetShaderResourceView(DxShaderResourceView, DxShader->GetShaderVisibility(), ParameterInfo.Register + i);
    }
}

void D3D12CommandContext::SetUnorderedAccessView(Shader* Shader, UnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetUnorderedAccessParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == 1);
    
    D3D12UnorderedAccessView* DxUnorderedAccessView = static_cast<D3D12UnorderedAccessView*>(UnorderedAccessView);
    DescriptorCache.SetUnorderedAccessView(DxUnorderedAccessView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
}

void D3D12CommandContext::SetUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetUnorderedAccessParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == NumUnorderedAccessViews);

    for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
    {
        D3D12UnorderedAccessView* DxUnorderedAccessView = static_cast<D3D12UnorderedAccessView*>(UnorderedAccessViews[i]);
        DescriptorCache.SetUnorderedAccessView(DxUnorderedAccessView, DxShader->GetShaderVisibility(), ParameterInfo.Register + i);
    }
}

void D3D12CommandContext::SetConstantBuffer(Shader* Shader, ConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetConstantBufferParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == 1);

    if (ConstantBuffer)
    {
        D3D12ConstantBufferView& DxConstantBufferView = static_cast<D3D12ConstantBuffer*>(ConstantBuffer)->GetView();
        DescriptorCache.SetConstantBufferView(&DxConstantBufferView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
    }
    else
    {
        DescriptorCache.SetConstantBufferView(nullptr, DxShader->GetShaderVisibility(), ParameterInfo.Register);
    }
}

void D3D12CommandContext::SetConstantBuffers(Shader* Shader, ConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetConstantBufferParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == NumConstantBuffers);

    for (uint32 i = 0; i < NumConstantBuffers; i++)
    {
        if (ConstantBuffers[i])
        {
            D3D12ConstantBufferView& DxConstantBufferView = static_cast<D3D12ConstantBuffer*>(ConstantBuffers[i])->GetView();
            DescriptorCache.SetConstantBufferView(&DxConstantBufferView, DxShader->GetShaderVisibility(), ParameterInfo.Register);
        }
        else
        {
            DescriptorCache.SetConstantBufferView(nullptr, DxShader->GetShaderVisibility(), ParameterInfo.Register);
        }
    }
}

void D3D12CommandContext::SetSamplerState(Shader* Shader, SamplerState* SamplerState, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetSamplerStateParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == 1);

    D3D12SamplerState* DxSamplerState = static_cast<D3D12SamplerState*>(SamplerState);
    DescriptorCache.SetSamplerState(DxSamplerState, DxShader->GetShaderVisibility(), ParameterInfo.Register);
}

void D3D12CommandContext::SetSamplerStates(Shader* Shader, SamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
{
    D3D12BaseShader* DxShader = D3D12ShaderCast(Shader);
    Assert(DxShader != nullptr);

    D3D12ShaderParameter ParameterInfo = DxShader->GetSamplerStateParameter(ParameterIndex);
    Assert(ParameterInfo.Space == 0);
    Assert(ParameterInfo.NumDescriptors == NumSamplerStates);

    for (uint32 i = 0; i < NumSamplerStates; i++)
    {
        D3D12SamplerState* DxSamplerState = static_cast<D3D12SamplerState*>(SamplerStates[i]);
        DescriptorCache.SetSamplerState(DxSamplerState, DxShader->GetShaderVisibility(), ParameterInfo.Register);
    }
}

void D3D12CommandContext::ResolveTexture(Texture* Destination, Texture* Source)
{
    FlushResourceBarriers();
 
    D3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
    D3D12BaseTexture* DxSource      = D3D12TextureCast(Source);
    const DXGI_FORMAT DstFormat = DxDestination->GetNativeFormat();
    const DXGI_FORMAT SrcFormat = DxSource->GetNativeFormat();
    
    //TODO: For now texture must be the same format. I.e typeless does probably not work
    Assert(DstFormat == SrcFormat);

    CmdList.ResolveSubresource(DxDestination->GetResource(), DxSource->GetResource(), DstFormat);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void D3D12CommandContext::UpdateBuffer(Buffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
    D3D12BaseBuffer* DxDestination = D3D12BufferCast(Destination);
    UpdateBuffer(DxDestination->GetResource(), OffsetInBytes, SizeInBytes, SourceData);

    CmdBatch->AddInUseResource(Destination);
}

void D3D12CommandContext::UpdateTexture2D(Texture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
    if (Width > 0 && Height > 0)
    {
        FlushResourceBarriers();

        D3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
        const DXGI_FORMAT NativeFormat  = DxDestination->GetNativeFormat();
        const uint32 Stride      = GetFormatStride(NativeFormat);
        const uint32 RowPitch    = ((Width * Stride) + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
        const uint32 SizeInBytes = Height * RowPitch;
    
        D3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();
        D3D12UploadAllocation Allocation = GpuResourceUploader.LinearAllocate(SizeInBytes);

        const uint8* Source = reinterpret_cast<const uint8*>(SourceData);
        for (uint32 y = 0; y < Height; y++)
        {
            Memory::Memcpy(Allocation.MappedPtr + (y * RowPitch), Source + (y * Width * Stride), Width * Stride);
        }

        // Copy to Dest
        D3D12_TEXTURE_COPY_LOCATION SourceLocation;
        Memory::Memzero(&SourceLocation);

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
        Memory::Memzero(&DestLocation);

        DestLocation.pResource        = DxDestination->GetResource()->GetResource();
        DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DestLocation.SubresourceIndex = MipLevel;

        CmdList.CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

        CmdBatch->AddInUseResource(Destination);
    }
}

void D3D12CommandContext::CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo)
{
    FlushResourceBarriers();

    D3D12BaseBuffer* DxDestination = D3D12BufferCast(Destination);
    D3D12BaseBuffer* DxSource      = D3D12BufferCast(Source);
    CmdList.CopyBufferRegion(DxDestination->GetResource(), CopyInfo.DestinationOffset, DxSource->GetResource(), CopyInfo.SourceOffset, CopyInfo.SizeInBytes);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void D3D12CommandContext::CopyTexture(Texture* Destination, Texture* Source)
{
    FlushResourceBarriers();

    D3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
    D3D12BaseTexture* DxSource      = D3D12TextureCast(Source);
    CmdList.CopyResource(DxDestination->GetResource(), DxSource->GetResource());

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void D3D12CommandContext::CopyTextureRegion(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyInfo)
{
    D3D12BaseTexture* DxDestination = D3D12TextureCast(Destination);
    D3D12BaseTexture* DxSource      = D3D12TextureCast(Source);

    // Source
    D3D12_TEXTURE_COPY_LOCATION SourceLocation;
    Memory::Memzero(&SourceLocation);

    SourceLocation.pResource        = DxSource->GetResource()->GetResource();
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
    Memory::Memzero(&DestinationLocation);

    DestinationLocation.pResource        = DxDestination->GetResource()->GetResource();
    DestinationLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestinationLocation.SubresourceIndex = CopyInfo.Destination.SubresourceIndex;

    FlushResourceBarriers();

    CmdList.CopyTextureRegion(&DestinationLocation, CopyInfo.Destination.x, CopyInfo.Destination.y, CopyInfo.Destination.z, &SourceLocation, &SourceBox);

    CmdBatch->AddInUseResource(Destination);
    CmdBatch->AddInUseResource(Source);
}

void D3D12CommandContext::DiscardResource(Resource* Resource)
{
    CmdBatch->AddInUseResource(Resource);
}

void D3D12CommandContext::BuildRayTracingGeometry(RayTracingGeometry* Geometry, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer, bool Update)
{
    FlushResourceBarriers();

    D3D12VertexBuffer* DxVertexBuffer   = static_cast<D3D12VertexBuffer*>(VertexBuffer);
    D3D12IndexBuffer*  DxIndexBuffer    = static_cast<D3D12IndexBuffer*>(IndexBuffer);
    D3D12RayTracingGeometry* DxGeometry = static_cast<D3D12RayTracingGeometry*>(Geometry);
    DxGeometry->VertexBuffer = DxVertexBuffer;
    DxGeometry->IndexBuffer  = DxIndexBuffer;
    DxGeometry->Build(*this, Update);

    CmdBatch->AddInUseResource(Geometry);
    CmdBatch->AddInUseResource(VertexBuffer);
    CmdBatch->AddInUseResource(IndexBuffer);
}

void D3D12CommandContext::BuildRayTracingScene(RayTracingScene* RayTracingScene, const RayTracingGeometryInstance* Instances, uint32 NumInstances, bool Update)
{
    FlushResourceBarriers();

    D3D12RayTracingScene* DxScene = static_cast<D3D12RayTracingScene*>(RayTracingScene);
    DxScene->Build(*this, Instances, NumInstances, Update);

    CmdBatch->AddInUseResource(RayTracingScene);
}

void D3D12CommandContext::SetRayTracingBindings(
    RayTracingScene* RayTracingScene, 
    RayTracingPipelineState* PipelineState, 
    const RayTracingShaderResources* GlobalResource, 
    const RayTracingShaderResources* RayGenLocalResources, 
    const RayTracingShaderResources* MissLocalResources, 
    const RayTracingShaderResources* HitGroupResources, 
    uint32 NumHitGroupResources)
{
    D3D12RayTracingScene*         DxScene         = static_cast<D3D12RayTracingScene*>(RayTracingScene);
    D3D12RayTracingPipelineState* DxPipelineState = static_cast<D3D12RayTracingPipelineState*>(PipelineState);
    Assert(DxScene != nullptr);
    Assert(DxPipelineState != nullptr);

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

    Assert(NumDescriptorsNeeded < D3D12_MAX_ONLINE_DESCRIPTOR_COUNT);
    D3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    if (!ResourceHeap->HasSpace(NumDescriptorsNeeded))
    {
        ResourceHeap->AllocateFreshHeap();
    }

    Assert(NumSamplersNeeded < D3D12_MAX_ONLINE_DESCRIPTOR_COUNT);
    D3D12OnlineDescriptorHeap* SamplerHeap = CmdBatch->GetOnlineSamplerDescriptorHeap();
    if (!SamplerHeap->HasSpace(NumSamplersNeeded))
    {
        SamplerHeap->AllocateFreshHeap();
    }

    if (!DxScene->BuildBindingTable(*this, DxPipelineState, ResourceHeap, SamplerHeap, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources))
    {
        LOG_ERROR("[D3D12CommandContext]: FAILED to Build Shader Binding Table");
    }

    Assert(GlobalResource != nullptr);

    if (!GlobalResource->ConstantBuffers.IsEmpty())
    {
        for (uint32 i = 0; i < GlobalResource->ConstantBuffers.Size(); i++)
        {
            D3D12ConstantBufferView& DxConstantBufferView = static_cast<D3D12ConstantBuffer*>(GlobalResource->ConstantBuffers[i])->GetView();
            DescriptorCache.SetConstantBufferView(&DxConstantBufferView, ShaderVisibility_All, i);
        }
    }
    if (!GlobalResource->ShaderResourceViews.IsEmpty())
    {
        for (uint32 i = 0; i < GlobalResource->ShaderResourceViews.Size(); i++)
        {
            D3D12ShaderResourceView* DxShaderResourceView = static_cast<D3D12ShaderResourceView*>(GlobalResource->ShaderResourceViews[i]);
            DescriptorCache.SetShaderResourceView(DxShaderResourceView, ShaderVisibility_All, i);
        }
    }
    if (!GlobalResource->UnorderedAccessViews.IsEmpty())
    {
        for (uint32 i = 0; i < GlobalResource->UnorderedAccessViews.Size(); i++)
        {
            D3D12UnorderedAccessView* DxUnorderedAccessView = static_cast<D3D12UnorderedAccessView*>(GlobalResource->UnorderedAccessViews[i]);
            DescriptorCache.SetUnorderedAccessView(DxUnorderedAccessView, ShaderVisibility_All, i);
        }
    }
    if (!GlobalResource->SamplerStates.IsEmpty())
    {
        for (uint32 i = 0; i < GlobalResource->SamplerStates.Size(); i++)
        {
            D3D12SamplerState* DxSampler = static_cast<D3D12SamplerState*>(GlobalResource->SamplerStates[i]);
            DescriptorCache.SetSamplerState(DxSampler, ShaderVisibility_All, i);
        }
    }

    ID3D12GraphicsCommandList4* DXRCommandList = CmdList.GetDXRCommandList();

    D3D12RootSignature* GlobalRootSignature = DxPipelineState->GetGlobalRootSignature();
    CurrentRootSignature = MakeSharedRef<D3D12RootSignature>(GlobalRootSignature);
    DXRCommandList->SetComputeRootSignature(CurrentRootSignature->GetRootSignature());

    DescriptorCache.CommitComputeDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());
}

void D3D12CommandContext::GenerateMips(Texture* Texture)
{
    D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
    Assert(DxTexture != nullptr);

    D3D12_RESOURCE_DESC Desc = DxTexture->GetResource()->GetDesc();
    Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    Assert(Desc.MipLevels > 1);

    // TODO: Create this placed from a Heap? See what performance is 
    TRef<D3D12Resource> StagingTexture = DBG_NEW D3D12Resource(GetDevice(), Desc, DxTexture->GetResource()->GetHeapType());
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
    const bool IsTextureCube = Texture->AsTextureCube();

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
    Memory::Memzero(&SrvDesc);

    SrvDesc.Format                  = Desc.Format;
    SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (IsTextureCube)
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
    Memory::Memzero(&UavDesc);

    UavDesc.Format = Desc.Format;
    if (IsTextureCube)
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
    const uint32 UavDescriptorHandleCount = Math::AlignUp<uint32>(Desc.MipLevels, MipLevelsPerDispatch);
    const uint32 NumDispatches            = UavDescriptorHandleCount / MipLevelsPerDispatch;

    D3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();

    // Allocate an extra handle for SRV
    const uint32 StartDescriptorHandleIndex         = ResourceHeap->AllocateHandles(UavDescriptorHandleCount + 1); 
    const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(StartDescriptorHandleIndex);
    GetDevice()->CreateShaderResourceView(DxTexture->GetResource()->GetResource(), &SrvDesc, SrvHandle_CPU);

    const uint32 UavStartDescriptorHandleIndex = StartDescriptorHandleIndex + 1;
    for (uint32 i = 0; i < Desc.MipLevels; i++)
    {
        if (IsTextureCube)
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
        if (IsTextureCube)
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

    if (IsTextureCube)
    {
        CmdList.SetPipelineState(GenerateMipsTexCube_PSO->GetPipeline());
        CmdList.SetComputeRootSignature(GenerateMipsTexCube_PSO->GetRootSignature());
    }
    else
    {
        CmdList.SetPipelineState(GenerateMipsTex2D_PSO->GetPipeline());
        CmdList.SetComputeRootSignature(GenerateMipsTex2D_PSO->GetRootSignature());
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE SrvHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(StartDescriptorHandleIndex);
    ID3D12DescriptorHeap* OnlineResourceHeap        = ResourceHeap->GetHeap()->GetHeap();    
    CmdList.SetDescriptorHeaps(&OnlineResourceHeap, 1);

    struct ConstantBuffer
    {
        uint32   SrcMipLevel;
        uint32   NumMipLevels;
        XMFLOAT2 TexelSize;
    } ConstantData;

    uint32 DstWidth  = static_cast<uint32>(Desc.Width);
    uint32 DstHeight = Desc.Height;
    ConstantData.SrcMipLevel = 0;

    const uint32 ThreadsZ     = IsTextureCube ? 6 : 1;
    uint32 RemainingMiplevels = Desc.MipLevels;
    for (uint32 i = 0; i < NumDispatches; i++)
    {
        ConstantData.TexelSize    = XMFLOAT2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));
        ConstantData.NumMipLevels = Math::Min<uint32>(4, RemainingMiplevels);

        CmdList.SetComputeRoot32BitConstants(&ConstantData, 4, 0, 0);
        
        // Because of DATA_STATIC_WHILE_SET_AT_EXECUTE error. Why we get this when we only use Root Signature 1.0 is beyond me.
        CmdList.SetComputeRootDescriptorTable(SrvHandle_GPU, 1);

        const uint32 GPUDescriptorHandleIndex           = i * MipLevelsPerDispatch;
        const D3D12_GPU_DESCRIPTOR_HANDLE UavHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(UavStartDescriptorHandleIndex + GPUDescriptorHandleIndex);
        CmdList.SetComputeRootDescriptorTable(UavHandle_GPU, 2);

        constexpr uint32 ThreadCount = 8;
        const uint32 ThreadsX = Math::DivideByMultiple(DstWidth, ThreadCount);
        const uint32 ThreadsY = Math::DivideByMultiple(DstHeight, ThreadCount);
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

        DstWidth  = DstWidth / 16;
        DstHeight = DstHeight / 16;

        ConstantData.SrcMipLevel += 3;
        RemainingMiplevels -= MipLevelsPerDispatch;
    }

    TransitionResource(DxTexture->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    FlushResourceBarriers();

    CmdBatch->AddInUseResource(Texture);
    CmdBatch->AddInUseResource(StagingTexture.Get());
}

void D3D12CommandContext::TransitionTexture(Texture* Texture, EResourceState BeforeState, EResourceState AfterState)
{
    const D3D12_RESOURCE_STATES DxBeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES DxAfterState  = ConvertResourceState(AfterState);

    D3D12BaseTexture* Resource = D3D12TextureCast(Texture);
    TransitionResource(Resource->GetResource(), DxBeforeState, DxAfterState);

    CmdBatch->AddInUseResource(Texture);
}

void D3D12CommandContext::TransitionBuffer(Buffer* Buffer, EResourceState BeforeState, EResourceState AfterState)
{
    const D3D12_RESOURCE_STATES DxBeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES DxAfterState  = ConvertResourceState(AfterState);
    
    D3D12BaseBuffer* Resource = D3D12BufferCast(Buffer);
    TransitionResource(Resource->GetResource(), DxBeforeState, DxAfterState);

    CmdBatch->AddInUseResource(Buffer);
}

void D3D12CommandContext::UnorderedAccessTextureBarrier(Texture* Texture)
{
    D3D12BaseTexture* Resource = D3D12TextureCast(Texture);
    UnorderedAccessBarrier(Resource->GetResource());

    CmdBatch->AddInUseResource(Texture);
}

void D3D12CommandContext::UnorderedAccessBufferBarrier(Buffer* Buffer)
{
    D3D12BaseBuffer* Resource = D3D12BufferCast(Buffer);
    UnorderedAccessBarrier(Resource->GetResource());

    CmdBatch->AddInUseResource(Buffer);
}

void D3D12CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    FlushResourceBarriers();

    ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
    DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

    CmdList.DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void D3D12CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    FlushResourceBarriers();

    ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
    DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());
    
    CmdList.DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void D3D12CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
    DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

    CmdList.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void D3D12CommandContext::DrawIndexedInstanced(
    uint32 IndexCountPerInstance, 
    uint32 InstanceCount, 
    uint32 StartIndexLocation, 
    uint32 BaseVertexLocation, 
    uint32 StartInstanceLocation)
{
    FlushResourceBarriers();

    ShaderConstantsCache.CommitGraphics(CmdList, CurrentRootSignature.Get());
    DescriptorCache.CommitGraphicsDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());

    CmdList.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void D3D12CommandContext::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    FlushResourceBarriers();

    ShaderConstantsCache.CommitCompute(CmdList, CurrentRootSignature.Get());
    DescriptorCache.CommitComputeDescriptors(CmdList, CmdBatch, CurrentRootSignature.Get());
    
    CmdList.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void D3D12CommandContext::DispatchRays(
    RayTracingScene* Scene,
    RayTracingPipelineState* PipelineState,
    uint32 Width,
    uint32 Height,
    uint32 Depth)
{
    D3D12RayTracingScene*         DxScene         = static_cast<D3D12RayTracingScene*>(Scene);
    D3D12RayTracingPipelineState* DxPipelineState = static_cast<D3D12RayTracingPipelineState*>(PipelineState);
    Assert(DxScene != nullptr);
    Assert(DxPipelineState != nullptr);

    ID3D12GraphicsCommandList4* DXRCommandList = CmdList.GetDXRCommandList();

    FlushResourceBarriers();

    D3D12_DISPATCH_RAYS_DESC RayDispatchDesc;
    Memory::Memzero(&RayDispatchDesc);

    RayDispatchDesc.RayGenerationShaderRecord = DxScene->GetRayGenShaderRecord();
    RayDispatchDesc.MissShaderTable           = DxScene->GetMissShaderTable();
    RayDispatchDesc.HitGroupTable             = DxScene->GetHitGroupTable();

    RayDispatchDesc.Width  = Width;
    RayDispatchDesc.Height = Height;
    RayDispatchDesc.Depth  = Depth;

    DXRCommandList->SetPipelineState1(DxPipelineState->GetStateObject());
    DXRCommandList->DispatchRays(&RayDispatchDesc);
}

void D3D12CommandContext::ClearState()
{
    Flush();

    for (D3D12CommandBatch& Batch : CmdBatches)
    {
        Batch.Reset();
    }

    InternalClearState();
}

void D3D12CommandContext::Flush()
{
    const uint64 NewFenceValue = ++FenceValue;
    if (!CmdQueue.SignalFence(Fence, NewFenceValue))
    {
        return;
    }

    Fence.WaitForValue(FenceValue);
}

void D3D12CommandContext::InsertMarker(const std::string& Message)
{
    if (SetMarkerOnCommandListFunc)
    {
        SetMarkerOnCommandListFunc(CmdList.GetGraphicsCommandList(), PIX_COLOR(255, 255, 255), Message.c_str());
    }
}

void D3D12CommandContext::BeginExternalCapture()
{
    IDXGraphicsAnalysis* PIX = GetDevice()->GetPIXCaptureInterface();
    if (PIX && !IsCapturing)
    {
        PIX->BeginCapture();
        IsCapturing = true;
    }
}

void D3D12CommandContext::EndExternalCapture()
{
    IDXGraphicsAnalysis* PIX = GetDevice()->GetPIXCaptureInterface();
    if (PIX && IsCapturing)
    {
        PIX->EndCapture();
        IsCapturing = false;
    }
}

void D3D12CommandContext::InternalClearState()
{
    DescriptorCache.Reset();
    ShaderConstantsCache.Reset();

    CurrentGraphicsPipelineState.Reset();
    CurrentRootSignature.Reset();
    CurrentComputePipelineState.Reset();
}
