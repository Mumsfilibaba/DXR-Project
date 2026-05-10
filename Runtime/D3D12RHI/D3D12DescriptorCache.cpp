#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/TypeTraits.h"
#include "D3D12RHI/D3D12DescriptorCache.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12CommandContext.h"

static TAutoConsoleVariable<int32> CVarSamplerDescriptorCacheSize(
    "D3D12RHI.SamplerDescriptorCacheSize",
    "Number of entries in the sampler descriptor LRU cache",
    256);

FD3D12LocalDescriptorHeap::FD3D12LocalDescriptorHeap(FD3D12Device* InDevice, FD3D12CommandContext& InContext, bool bInSamplers)
    : FD3D12DeviceChild(InDevice)
    , Context(InContext)
    , Heap(nullptr)
    , Block(nullptr)
    , CurrentHandle(0)
    , bIsSamplerHeap(bInSamplers)
{
}

bool FD3D12LocalDescriptorHeap::Initialize()
{
    Realloc();
    return Block != nullptr;
}

uint32 FD3D12LocalDescriptorHeap::AllocateHandles(uint32 NumHandles)
{
    CHECK(HasSpace(NumHandles));
        
    const uint32 NewOffset = CurrentHandle + NumHandles;
    if (NewOffset > Block->NumDescriptors)
    {
        return static_cast<uint32>(-1);
    }

    const uint32 Result = CurrentHandle;
    CurrentHandle = NewOffset;
    return Result;
}

bool FD3D12LocalDescriptorHeap::Realloc()
{
    // Delete the old block if it exists
    FD3D12OnlineDescriptorHeap& GlobalHeap = bIsSamplerHeap ? GetDevice()->GetGlobalSamplerHeap() : GetDevice()->GetGlobalResourceHeap();
    if (Block)
    {
        GlobalHeap.RecycleBlockDeferred(Block);
        Block = nullptr;
        Heap.Reset();
    }

    CHECK(Block == nullptr);
    
    Block         = GlobalHeap.AllocateBlock();
    CurrentHandle = 0;

    if (Block)
    {
        Heap = new FD3D12DescriptorHeap(GlobalHeap.GetHeap(), Block->HandleOffset, Block->NumDescriptors);
        return true;
    }

    return false;
}

bool FD3D12LocalDescriptorHeap::HasSpace(uint32 NumHandles) const
{
    if (!Block)
    {
        return false;
    }

    if (NumHandles > Block->NumDescriptors)
    {
        return false;
    }

    const uint32 NewOffset = CurrentHandle + NumHandles;
    if (NewOffset > Block->NumDescriptors)
    {
        return false;
    }

    return true;
}

FD3D12DescriptorCache::FD3D12DescriptorCache(FD3D12Device* InDevice, FD3D12CommandContext& InContext)
    : FD3D12DeviceChild(InDevice)
    , Context(InContext)
    , DefaultDescriptors(InDevice->GetDefaultDescriptors())
    , SamplerCache(Math::Max<int32>(16, CVarSamplerDescriptorCacheSize.GetValue()))
    , ResourceHeap(InDevice, InContext, false)
    , SamplerHeap(InDevice, InContext, true)
{
}

bool FD3D12DescriptorCache::Initialize()
{
    if (!ResourceHeap.Initialize())
    {
        DEBUG_BREAK();
        return false;
    }

    if (!SamplerHeap.Initialize())
    {
        DEBUG_BREAK();
        return false;
    }

    // Start by resetting the descriptor cache
    DirtyState();
    return true;
}

void FD3D12DescriptorCache::DirtyState()
{
    CurrentDescriptorHeaps[0] = nullptr;
    CurrentDescriptorHeaps[1] = nullptr;

    ConstantBufferCache.ClearAll();
    ShaderResourceViewCache.ClearAll();
    UnorderedAccessViewCache.ClearAll();
    SamplerDescriptorHandles.ClearAll();
}

void FD3D12DescriptorCache::DirtyStateSamplers()
{
    SamplerDescriptorHandles.ClearAll();
}

void FD3D12DescriptorCache::DirtyStateResources()
{
    ConstantBufferCache.ClearAll();
    ShaderResourceViewCache.ClearAll();
    UnorderedAccessViewCache.ClearAll();
}

void FD3D12DescriptorCache::InvalidateCachedSamplerTables()
{
    SamplerCache.Clear();
}

void FD3D12DescriptorCache::SetRenderTargets(FD3D12RenderTargetCache& Cache)
{
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandles[D3D12_MAX_RENDER_TARGET_COUNT];
    for (uint32 Index = 0; Index < Cache.NumRenderTargets; ++Index)
    {
        if (FD3D12RenderTargetViewRHI* CurrentView = Cache.RenderTargetViews[Index])
        {
            RenderTargetViewHandles[Index] = CurrentView->GetOfflineHandle();
            Context.GetCommandList().UpdateResidency(CurrentView->GetResourceResidencyHandle());
        }
        else
        {
            RenderTargetViewHandles[Index] = DefaultDescriptors.DefaultRTV->GetOfflineHandle();
        }
    }

    if (Cache.DepthStencilView)
    {
        Context.GetCommandList().UpdateResidency(Cache.DepthStencilView->GetResourceResidencyHandle());

        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle = Cache.DepthStencilView->GetOfflineHandle();
        Context.GetCommandList()->OMSetRenderTargets(Cache.NumRenderTargets, RenderTargetViewHandles, false, &DepthStencilHandle);
    }
    else
    {
        Context.GetCommandList()->OMSetRenderTargets(Cache.NumRenderTargets, RenderTargetViewHandles, false, nullptr);
    }
}

void FD3D12DescriptorCache::SetVertexBuffers(FD3D12VertexBufferCache& VertexBuffers)
{
    if (VertexBuffers.NumVertexBuffers != 0)
    {
        for (uint32 i = 0; i < VertexBuffers.NumVertexBuffers; i++)
        {
            if (FD3D12BufferRHI* Buffer = VertexBuffers.BufferResources[i])
            {
                Context.GetCommandList().UpdateResidency(Buffer->GetResource()->GetResidencyHandle());
            }
        }

        Context.GetCommandList()->IASetVertexBuffers(0, VertexBuffers.NumVertexBuffers, VertexBuffers.VertexBuffers);
    }
}

void FD3D12DescriptorCache::SetIndexBuffer(FD3D12IndexBufferCache& IndexBuffer)
{
    if (FD3D12BufferRHI* Buffer = IndexBuffer.BufferResource)
    {
        Context.GetCommandList().UpdateResidency(Buffer->GetResource()->GetResidencyHandle());
    }

    Context.GetCommandList()->IASetIndexBuffer(&IndexBuffer.IndexBuffer);
}

void FD3D12DescriptorCache::PrepareCBVs(FD3D12ConstantBufferCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage, uint32 NumCBVs, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::CBV);
    if (ParameterIndex < 0)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    if (!NumCBVs)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_CONSTANT_BUFFER_COUNT];

    const FD3D12DescriptorTableMapping& Mapping = RootSignature->GetDescriptorTableMapping(ShaderStage, EResourceType::CBV);
    auto& CBVCache = Cache.ResourceViews[ShaderStage];
    for (uint32 Slot = 0; Slot < NumCBVs; Slot++)
    {
        const uint16 Register = Mapping.GetRegisterForSlot(static_cast<uint8>(Slot));
        CHECK(Register < D3D12_DEFAULT_CONSTANT_BUFFER_COUNT);

        if (FD3D12BufferRHI* Buffer = CBVCache[Register])
        {
            if (FD3D12ConstantBufferView* View = Buffer->GetOrCreateConstantBufferView())
            {
                OfflineHandles[Slot] = View->GetOfflineHandle();
                Context.GetCommandList().UpdateResidency(View->GetResourceResidencyHandle());
            }
            else
            {
                OfflineHandles[Slot] = DefaultDescriptors.DefaultCBV->GetOfflineHandle();
            }
        }
        else
        {
            OfflineHandles[Slot] = DefaultDescriptors.DefaultCBV->GetOfflineHandle();
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle = ResourceHeap.GetCPUHandle(DescriptorHandleOffset);
    ConstantBufferCache.Handles[ShaderStage] = ResourceHeap.GetGPUHandle(DescriptorHandleOffset);
    DescriptorHandleOffset += NumCBVs;

    const UINT DestRangeSize = NumCBVs;
    GetDevice()->GetD3D12Device()->CopyDescriptors(
        1,
        &OnlineHandle,
        &DestRangeSize,
        NumCBVs,
        OfflineHandles,
        nullptr,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Cache.ClearResourcesDirty(ShaderStage);
    Cache.DirtyDescriptorTable(ShaderStage);
}

void FD3D12DescriptorCache::BindCBVs(FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::CBV);
    if (ParameterIndex < 0)
    {
        return;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = ConstantBufferCache.Handles[ShaderStage];
    if (GPUDescriptorHandle.ptr == 0)
    {
        return;
    }

    if (ShaderStage == EShaderVisibility::All)
    {
        Context.GetCommandList()->SetComputeRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
}

void FD3D12DescriptorCache::PrepareSRVs(FD3D12ShaderResourceViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage, uint32 NumSRVs, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::SRV);
    if (ParameterIndex < 0)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    if (!NumSRVs)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];

    const FD3D12DescriptorTableMapping& Mapping = RootSignature->GetDescriptorTableMapping(ShaderStage, EResourceType::SRV);
    auto& SRVCache = Cache.ResourceViews[ShaderStage];
    for (uint32 Slot = 0; Slot < NumSRVs; Slot++)
    {
        const uint16 Register = Mapping.GetRegisterForSlot(static_cast<uint8>(Slot));
        CHECK(Register < D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

        if (FD3D12ShaderResourceViewRHI* ShaderResourceView = SRVCache[Register])
        {
            OfflineHandles[Slot] = ShaderResourceView->GetOfflineHandle();
            Context.GetCommandList().UpdateResidency(ShaderResourceView->GetResourceResidencyHandle());
        }
        else
        {
            OfflineHandles[Slot] = DefaultDescriptors.DefaultSRV->GetOfflineHandle();
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle     = ResourceHeap.GetCPUHandle(DescriptorHandleOffset);
    ShaderResourceViewCache.Handles[ShaderStage] = ResourceHeap.GetGPUHandle(DescriptorHandleOffset);
    DescriptorHandleOffset += NumSRVs;

    const UINT DestRangeSize = NumSRVs;
    GetDevice()->GetD3D12Device()->CopyDescriptors(
        1,
        &OnlineHandle,
        &DestRangeSize,
        NumSRVs,
        OfflineHandles,
        nullptr,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Cache.ClearResourcesDirty(ShaderStage);
    Cache.DirtyDescriptorTable(ShaderStage);
}

void FD3D12DescriptorCache::BindSRVs(FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::SRV);
    if (ParameterIndex < 0)
    {
        return;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = ShaderResourceViewCache.Handles[ShaderStage];
    if (GPUDescriptorHandle.ptr == 0)
    {
        return;
    }

    if (ShaderStage == EShaderVisibility::All)
    {
        Context.GetCommandList()->SetComputeRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
}

void FD3D12DescriptorCache::PrepareUAVs(FD3D12UnorderedAccessViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage, uint32 NumUAVs, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::UAV);
    if (ParameterIndex < 0)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    if (!NumUAVs)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];

    const FD3D12DescriptorTableMapping& Mapping = RootSignature->GetDescriptorTableMapping(ShaderStage, EResourceType::UAV);
    auto& UAVCache = Cache.ResourceViews[ShaderStage];
    for (uint32 Slot = 0; Slot < NumUAVs; Slot++)
    {
        const uint16 Register = Mapping.GetRegisterForSlot(static_cast<uint8>(Slot));
        CHECK(Register < D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

        if (FD3D12UnorderedAccessViewRHI* UnorderedAccessView = UAVCache[Register])
        {
            OfflineHandles[Slot] = UnorderedAccessView->GetOfflineHandle();
            Context.GetCommandList().UpdateResidency(UnorderedAccessView->GetResourceResidencyHandle());
        }
        else
        {
            OfflineHandles[Slot] = DefaultDescriptors.DefaultUAV->GetOfflineHandle();
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle      = ResourceHeap.GetCPUHandle(DescriptorHandleOffset);
    UnorderedAccessViewCache.Handles[ShaderStage] = ResourceHeap.GetGPUHandle(DescriptorHandleOffset);
    DescriptorHandleOffset += NumUAVs;

    const UINT DestRangeSize = NumUAVs;
    GetDevice()->GetD3D12Device()->CopyDescriptors(
        1,
        &OnlineHandle,
        &DestRangeSize,
        NumUAVs,
        OfflineHandles,
        nullptr,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Cache.ClearResourcesDirty(ShaderStage);
    Cache.DirtyDescriptorTable(ShaderStage);
}

void FD3D12DescriptorCache::BindUAVs(FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::UAV);
    if (ParameterIndex < 0)
    {
        return;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = UnorderedAccessViewCache.Handles[ShaderStage];
    if (GPUDescriptorHandle.ptr == 0)
    {
        return;
    }

    if (ShaderStage == EShaderVisibility::All)
    {
        Context.GetCommandList()->SetComputeRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
}

void FD3D12DescriptorCache::PrepareSamplers(FD3D12SamplerStateCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage, uint32 NumSamplers, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::Sampler);
    if (ParameterIndex < 0)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    if (!NumSamplers)
    {
        Cache.ClearResourcesDirty(ShaderStage);
        Cache.DirtyDescriptorTable(ShaderStage);
        return;
    }

    FD3D12UniqueSamplerTable UniqueTable;

    const FD3D12DescriptorTableMapping& Mapping = RootSignature->GetDescriptorTableMapping(ShaderStage, EResourceType::Sampler);
    auto& SamplerStates = Cache.SamplerStates[ShaderStage];
    for (uint32 Slot = 0; Slot < NumSamplers; Slot++)
    {
        const uint16 Register = Mapping.GetRegisterForSlot(static_cast<uint8>(Slot));
        CHECK(Register < D3D12_DEFAULT_SAMPLER_STATE_COUNT);

        if (FD3D12SamplerStateRHI* SamplerState = SamplerStates[Register])
        {
            UniqueTable.UniqueIDs[Slot] = SamplerState->GetUniqueID().Identifier;
        }
        else
        {
            UniqueTable.UniqueIDs[Slot] = DefaultDescriptors.DefaultSampler->GetUniqueID().Identifier;
        }
    }

    D3D12_GPU_DESCRIPTOR_HANDLE* CachedTable = SamplerCache.Find(UniqueTable);
    if (!CachedTable)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_SAMPLER_STATE_COUNT];
        for (uint32 Slot = 0; Slot < NumSamplers; Slot++)
        {
            const uint16 Register = Mapping.GetRegisterForSlot(static_cast<uint8>(Slot));
            CHECK(Register < D3D12_DEFAULT_SAMPLER_STATE_COUNT);

            if (FD3D12SamplerStateRHI* SamplerState = SamplerStates[Register])
            {
                OfflineHandles[Slot] = SamplerState->GetOfflineHandle();
            }
            else
            {
                OfflineHandles[Slot] = DefaultDescriptors.DefaultSampler->GetOfflineHandle();
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle    = SamplerHeap.GetCPUHandle(DescriptorHandleOffset);
        D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandleGPU = SamplerHeap.GetGPUHandle(DescriptorHandleOffset);
        DescriptorHandleOffset += NumSamplers;

        const UINT DestRangeSize = NumSamplers;
        GetDevice()->GetD3D12Device()->CopyDescriptors(
            1,
            &OnlineHandle,
            &DestRangeSize,
            NumSamplers,
            OfflineHandles,
            nullptr,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        SamplerDescriptorHandles.Handles[ShaderStage] = OnlineHandleGPU;
        SamplerCache.Insert(OnlineHandleGPU, UniqueTable);
    }
    else
    {
        SamplerDescriptorHandles.Handles[ShaderStage] = *CachedTable;
    }

    Cache.ClearResourcesDirty(ShaderStage);
    Cache.DirtyDescriptorTable(ShaderStage);
}

void FD3D12DescriptorCache::BindSamplers(FD3D12RootSignature* RootSignature, EShaderVisibility::Type ShaderStage)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::Sampler);
    if (ParameterIndex < 0)
    {
        return;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = SamplerDescriptorHandles.Handles[ShaderStage];
    if (GPUDescriptorHandle.ptr == 0)
    {
        return;
    }

    if (ShaderStage == EShaderVisibility::All)
    {
        Context.GetCommandList()->SetComputeRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
}

void FD3D12DescriptorCache::SetDescriptorHeaps()
{
    ID3D12DescriptorHeap* DescriptorHeaps[] =
    {
        ResourceHeap.GetHeap()->GetD3D12Heap(),
        SamplerHeap.GetHeap()->GetD3D12Heap()
    };

    if (CurrentDescriptorHeaps[0] != DescriptorHeaps[0] || CurrentDescriptorHeaps[1] != DescriptorHeaps[1] || GD3D12ForceBinding)
    {
        Context.GetCommandList()->SetDescriptorHeaps(ARRAY_COUNT(DescriptorHeaps), DescriptorHeaps);

        CurrentDescriptorHeaps[0] = DescriptorHeaps[0];
        CurrentDescriptorHeaps[1] = DescriptorHeaps[1];
    }
}
