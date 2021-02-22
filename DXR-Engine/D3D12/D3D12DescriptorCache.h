#pragma once
#include "D3D12RootSignature.h"
#include "D3D12Views.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12SamplerState.h"
#include "D3D12CommandList.h"

#define NUM_VISIBILITIES (ShaderVisibility_Count)
#define NUM_DESCRIPTORS  (D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT)
#define NUM_DIRTY_MASKS  (NUM_DESCRIPTORS / 64)

template <typename TD3D12DescriptorType>
struct TD3D12DescriptorCache
{
    TD3D12DescriptorCache()
        : Descriptors()
        , DirtyMasks()
        , DescriptorRangeLengths()
        , Dirty(false)
    {
        Reset();
    }

    void Set(TD3D12DescriptorType* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        Assert(Descriptor != nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE  NewHandle     = Descriptor->GetOfflineHandle();
        D3D12_CPU_DESCRIPTOR_HANDLE& CurrentHandle = Descriptors[Visibility][ShaderRegister];
        if (NewHandle != CurrentHandle)
        {
            CurrentHandle = NewHandle;
        
            UInt64 MaskIndex = ShaderRegister / NUM_DIRTY_MASKS;
            UInt64 Bit       = ShaderRegister % NUM_DIRTY_MASKS;
            UInt64& Mask = DirtyMasks[Visibility][MaskIndex];
            Mask |= BIT(Bit);

            UInt64& RangeLength = DescriptorRangeLengths[Visibility];
            RangeLength = Math::Max<UInt32>(RangeLength, ShaderRegister + 1);

            Dirty = true;
        }
    }

    void Reset()
    {
        Memory::Memzero(DirtyMasks, sizeof(DirtyMasks));
        Memory::Memzero(Descriptors, sizeof(Descriptors));
        Memory::Memzero(DescriptorRangeLengths, sizeof(DescriptorRangeLengths));
        Dirty = false;
    }

    Bool IsDirty() const { return Dirty; }

    // NOTE: This is ALOT of memory, we should probably refactor this, maybe a class that 
    //       handles descriptorbindings. class ResourceTable? That can be stored outside the renderlayer
    //       problem is how we deal with ClearUnorderedAccessView, should we change heap when this happens?
    //       However this would better support unbound resources.
    D3D12_CPU_DESCRIPTOR_HANDLE Descriptors[NUM_VISIBILITIES][NUM_DESCRIPTORS];
    UInt64 DirtyMasks[NUM_VISIBILITIES][NUM_DIRTY_MASKS];
    UInt64 DescriptorRangeLengths[NUM_VISIBILITIES];
    Bool Dirty;
};

using D3D12ConstantBufferViewCache  = TD3D12DescriptorCache<D3D12ConstantBufferView>;
using D3D12ShaderResourceViewCache  = TD3D12DescriptorCache<D3D12ShaderResourceView>;
using D3D12UnorderedAccessViewCache = TD3D12DescriptorCache<D3D12UnorderedAccessView>;
using D3D12SamplerStateCache        = TD3D12DescriptorCache<D3D12SamplerState>;

class D3D12VertexBufferCache
{
public:
    D3D12VertexBufferCache()
        : VertexBufferViews()
        , NumVertexBuffers(0)
        , IndexBufferView()
        , IndexBufferDirty(false)
    {
        Reset();
    }

    FORCEINLINE void SetVertexBuffer(D3D12VertexBuffer* VertexBuffer, UInt32 Slot)
    {
        Assert(Slot < D3D12_MAX_VERTEX_BUFFER_SLOTS);

        if (!VertexBuffer)
        {
            VertexBufferViews[Slot].BufferLocation = 0;
            VertexBufferViews[Slot].SizeInBytes    = 0;
            VertexBufferViews[Slot].StrideInBytes  = 0;
        }
        else
        {
            // TODO: Maybe save a ref so that we can ensure that the buffer
            //       does not get deleted until commandbatch is finished
            VertexBufferViews[Slot] = VertexBuffer->GetView();
        }

        NumVertexBuffers = Math::Max(NumVertexBuffers, Slot + 1);
    }

    FORCEINLINE void SetIndexBuffer(D3D12IndexBuffer* InIndexBuffer)
    {
        if (!InIndexBuffer)
        {
            SetNullIndexBuffer(IndexBufferView);
        }
        else
        {
            IndexBufferView = InIndexBuffer->GetView();
        }

        IndexBufferDirty = true;
    }

    FORCEINLINE void CommitState(D3D12CommandListHandle& CmdList)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        DxCmdList->IASetVertexBuffers(0, NumVertexBuffers, VertexBufferViews);

        if (IndexBufferDirty)
        {
            DxCmdList->IASetIndexBuffer(&IndexBufferView);
            IndexBufferDirty = false;
        }
    }

    FORCEINLINE void Reset()
    {
        Memory::Memzero(VertexBufferViews, sizeof(VertexBufferViews));
        NumVertexBuffers = 0;

        SetNullIndexBuffer(IndexBufferView);
    }

private:
    static void SetNullIndexBuffer(D3D12_INDEX_BUFFER_VIEW& OutIndexBuffer)
    {
        OutIndexBuffer.Format         = DXGI_FORMAT_R32_UINT;
        OutIndexBuffer.BufferLocation = 0;
        OutIndexBuffer.SizeInBytes    = 0;
    }

    D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    UInt32 NumVertexBuffers;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    Bool IndexBufferDirty;
};

class D3D12RenderTargetState
{
public:
    D3D12RenderTargetState()
        : RenderTargetViewHandles()
        , DepthStencilViewHandle({ 0 })
        , Dirty(false)
    {
        Reset();
    }

    FORCEINLINE void SetRenderTargetView(D3D12RenderTargetView* RenderTargetView, UInt32 Slot)
    {
        Assert(Slot < D3D12_MAX_RENDER_TARGET_COUNT);

        if (RenderTargetView)
        {
            RenderTargetViewHandles[Slot] = RenderTargetView->GetOfflineHandle();
        }
        else
        {
            RenderTargetViewHandles[Slot] = { 0 };
        }

        NumRenderTargets = Math::Max(NumRenderTargets, Slot + 1);
        Dirty = true; 
    }

    FORCEINLINE void SetDepthStencilView(D3D12DepthStencilView* DepthStencilView)
    {
        if (DepthStencilView)
        {
            DepthStencilViewHandle = DepthStencilView->GetOfflineHandle();
        }
        else
        {
            DepthStencilViewHandle = { 0 };
        }

        Dirty = true; 
    }

    FORCEINLINE void Reset()
    {
        Memory::Memzero(RenderTargetViewHandles, sizeof(RenderTargetViewHandles));
        DepthStencilViewHandle = { 0 };
        NumRenderTargets = 0;
    }

    FORCEINLINE void CommitState(D3D12CommandListHandle& CmdList)
    {
        if (Dirty)
        {
            ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
            DxCmdList->OMSetRenderTargets(NumRenderTargets, RenderTargetViewHandles, FALSE, &DepthStencilViewHandle);
            Dirty = false;
        }
    }

    FORCEINLINE const D3D12_CPU_DESCRIPTOR_HANDLE* GetDepthStencilHandle() const
    {
        if (DepthStencilViewHandle.ptr != 0)
        {
            return &DepthStencilViewHandle;
        }
        else
        {
            return nullptr;
        }
    }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandles[D3D12_MAX_RENDER_TARGET_COUNT];
    UInt32 NumRenderTargets;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilViewHandle;
    Bool Dirty;
};

class D3D12DescriptorCache : public D3D12DeviceChild
{
public:
    D3D12DescriptorCache(D3D12Device* Device);
    ~D3D12DescriptorCache();

    Bool Init();

    void SetConstantBufferView(D3D12ConstantBufferView* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullCBV;
        }

        ConstantBufferViewCache.Set(Descriptor, Visibility, ShaderRegister);
    }

    void SetShaderResourceView(D3D12ShaderResourceView* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSRV;
        }

        ShaderResourceViewCache.Set(Descriptor, Visibility, ShaderRegister);
    }

    void SetUnorderedAccessView(D3D12UnorderedAccessView* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullUAV;
        }

        UnorderedAccessViewCache.Set(Descriptor, Visibility, ShaderRegister);
    }

    void SetSamplerState(D3D12SamplerState* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSampler;
        }

        SamplerStateCache.Set(Descriptor, Visibility, ShaderRegister);
    }

    void CommitGraphicsDescriptorTables(D3D12CommandListHandle& CmdList, class D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature);
    void CommitComputeDescriptorTables(D3D12CommandListHandle& CmdList, class D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature);

    void Reset();

private:
    void CopyDescriptors(D3D12CommandListHandle& CmdList, class D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature);
    
    D3D12ConstantBufferView*  NullCBV     = nullptr;
    D3D12ShaderResourceView*  NullSRV     = nullptr;
    D3D12UnorderedAccessView* NullUAV     = nullptr;
    D3D12SamplerState*        NullSampler = nullptr;

    D3D12VertexBufferCache        VertexBufferCache;
    D3D12RenderTargetState        RenderTargetCache;
    D3D12ShaderResourceViewCache  ShaderResourceViewCache;
    D3D12UnorderedAccessViewCache UnorderedAccessViewCache;
    D3D12ConstantBufferViewCache  ConstantBufferViewCache;
    D3D12SamplerStateCache        SamplerStateCache;

    UINT RangeSizes[NUM_DESCRIPTORS];
};