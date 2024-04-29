#include "D3D12DescriptorCache.h"
#include "D3D12Descriptors.h"
#include "D3D12RHI.h"
#include "D3D12CommandContext.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/TypeTraits.h"

FD3D12LocalDescriptorHeap::FD3D12LocalDescriptorHeap(FD3D12Device* InDevice, FD3D12CommandContext& InContext, bool bInSamplers)
    : FD3D12DeviceChild(InDevice)
    , Context(InContext)
    , Heap(nullptr)
    , Block(nullptr)
    , CurrentHandle(0)
    , bSamplers(bInSamplers)
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
    FD3D12OnlineDescriptorHeap& GlobalHeap = bSamplers ? GetDevice()->GetGlobalSamplerHeap() : GetDevice()->GetGlobalResourceHeap();
    if (Block)
    {
        GlobalHeap.FreeBlockDeferred(Block);
        Block = nullptr;
        Heap.Reset();
    }

    CHECK(Block == nullptr);
    Block = GlobalHeap.AllocateBlock();
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
    if (NewOffset >= Block->NumDescriptors)
    {
        return false;
    }

    return true;
}


FD3D12DescriptorCache::FD3D12DescriptorCache(FD3D12Device* InDevice, FD3D12CommandContext& InContext)
    : FD3D12DeviceChild(InDevice)
    , Context(InContext)
    , DefaultDescriptors(InDevice->GetDefaultDescriptors())
    , SamplerCache(256)
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

void FD3D12DescriptorCache::SetRenderTargets(FD3D12RenderTargetCache& Cache)
{
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandles[D3D12_MAX_RENDER_TARGET_COUNT];
    for (uint32 Index = 0; Index < Cache.NumRenderTargets; ++Index)
    {
        if (FD3D12RenderTargetView* CurrentView = Cache.RenderTargetViews[Index])
        {
            RenderTargetViewHandles[Index] = CurrentView->GetOfflineHandle();
        }
        else
        {
            RenderTargetViewHandles[Index] = DefaultDescriptors.DefaultRTV->GetOfflineHandle();
        }
    }

    if (Cache.DepthStencilView)
    {
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
        Context.GetCommandList()->IASetVertexBuffers(0, VertexBuffers.NumVertexBuffers, VertexBuffers.VertexBuffers);
    }
}

void FD3D12DescriptorCache::SetIndexBuffer(FD3D12IndexBufferCache& IndexBuffer)
{
    Context.GetCommandList()->IASetIndexBuffer(&IndexBuffer.IndexBuffer);
}

void FD3D12DescriptorCache::SetCBVs(FD3D12ConstantBufferCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumCBVs, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::ResourceType_CBV);
    if (ParameterIndex < 0)
    {
        return;
    }

    if (Cache.IsDirty(ShaderStage) || GD3D12ForceBinding)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_CONSTANT_BUFFER_COUNT];

        auto& CBVCache = Cache.ResourceViews[ShaderStage];
        for (int32 Index = 0; Index < NumCBVs; Index++)
        {
            if (FD3D12ConstantBufferView* ConstantBuffer = CBVCache[Index])
            {
                OfflineHandles[Index] = ConstantBuffer->GetOfflineHandle();
            }
            else
            {
                OfflineHandles[Index] = DefaultDescriptors.DefaultCBV->GetOfflineHandle();
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

        Cache.bDirty[ShaderStage] = false;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = ConstantBufferCache.Handles[ShaderStage];
    CHECK(GPUDescriptorHandle.ptr != 0);

    if (ShaderStage == ShaderVisibility_All)
    {
        Context.GetCommandList()->SetComputeRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
}

void FD3D12DescriptorCache::SetSRVs(FD3D12ShaderResourceViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumSRVs, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::ResourceType_SRV);
    if (ParameterIndex < 0)
    {
        return;
    }

    if (Cache.IsDirty(ShaderStage) || GD3D12ForceBinding)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];

        auto& SRVCache = Cache.ResourceViews[ShaderStage];
        for (int32 Index = 0; Index < NumSRVs; Index++)
        {
            if (FD3D12ShaderResourceView* ShaderResourceView = SRVCache[Index])
            {
                OfflineHandles[Index] = ShaderResourceView->GetOfflineHandle();
            }
            else
            {
                OfflineHandles[Index] = DefaultDescriptors.DefaultSRV->GetOfflineHandle();
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

        Cache.bDirty[ShaderStage] = false;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = ShaderResourceViewCache.Handles[ShaderStage];
    CHECK(GPUDescriptorHandle.ptr != 0);

    if (ShaderStage == ShaderVisibility_All)
    {
        Context.GetCommandList()->SetComputeRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
}

void FD3D12DescriptorCache::SetUAVs(FD3D12UnorderedAccessViewCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumUAVs, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::ResourceType_UAV);
    if (ParameterIndex < 0)
    {
        return;
    }

    if (Cache.IsDirty(ShaderStage) || GD3D12ForceBinding)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];

        auto& UAVCache = Cache.ResourceViews[ShaderStage];
        for (int32 Index = 0; Index < NumUAVs; Index++)
        {
            if (FD3D12UnorderedAccessView* UnorderedAccessView = UAVCache[Index])
            {
                OfflineHandles[Index] = UnorderedAccessView->GetOfflineHandle();
            }
            else
            {
                OfflineHandles[Index] = DefaultDescriptors.DefaultUAV->GetOfflineHandle();
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

        Cache.bDirty[ShaderStage] = false;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = UnorderedAccessViewCache.Handles[ShaderStage];
    CHECK(GPUDescriptorHandle.ptr != 0);

    if (ShaderStage == ShaderVisibility_All)
    {
        Context.GetCommandList()->SetComputeRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
    else
    {
        Context.GetCommandList()->SetGraphicsRootDescriptorTable(ParameterIndex, GPUDescriptorHandle);
    }
}

void FD3D12DescriptorCache::SetSamplers(FD3D12SamplerStateCache& Cache, FD3D12RootSignature* RootSignature, EShaderVisibility ShaderStage, uint32 NumSamplers, uint32& DescriptorHandleOffset)
{
    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderStage, EResourceType::ResourceType_Sampler);
    if (ParameterIndex < 0)
    {
        return;
    }

    if (Cache.IsDirty(ShaderStage) || GD3D12ForceBinding)
    {
        FD3D12UniqueSamplerTable UniqueTable;

        auto& SamplerStates = Cache.SamplerStates[ShaderStage];
        for (int32 Index = 0; Index < NumSamplers; Index++)
        {
            if (FD3D12SamplerState* SamplerState = SamplerStates[Index])
            {
                UniqueTable.UniqueIDs[Index] = SamplerState->GetUniqueID().Identifer;
            }
            else
            {
                UniqueTable.UniqueIDs[Index] = DefaultDescriptors.DefaultSampler->GetUniqueID().Identifer;
            }
        }

        D3D12_GPU_DESCRIPTOR_HANDLE* CachedTable = SamplerCache.Find(UniqueTable);
        if (!CachedTable)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandles[D3D12_DEFAULT_SAMPLER_STATE_COUNT];
            for (int32 Index = 0; Index < NumSamplers; Index++)
            {
                if (FD3D12SamplerState* SamplerState = SamplerStates[Index])
                {
                    OfflineHandles[Index] = SamplerState->GetOfflineHandle();
                }
                else
                {
                    OfflineHandles[Index] = DefaultDescriptors.DefaultSampler->GetOfflineHandle();
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

        Cache.bDirty[ShaderStage] = false;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle = SamplerDescriptorHandles.Handles[ShaderStage];
    CHECK(GPUDescriptorHandle.ptr != 0);

    if (ShaderStage == ShaderVisibility_All)
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
