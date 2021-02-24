#pragma once
#include "D3D12RootSignature.h"
#include "D3D12Views.h"
#include "D3D12Buffer.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12SamplerState.h"
#include "D3D12CommandList.h"

#define NUM_VISIBILITIES (ShaderVisibility_Count)
#define NUM_DESCRIPTORS  (D3D12_MAX_ONLINE_DESCRIPTOR_COUNT / 4)

template <typename TD3D12DescriptorViewType>
struct TD3D12DescriptorViewCache
{
    TD3D12DescriptorViewCache()
        : DescriptorViews()
        , Descriptors()
        , CopyDescriptors()
        , Dirty()
        , DescriptorRangeLengths()
        , TotalNumDescriptors(0)
    {
        Reset();
    }

    void Set(TD3D12DescriptorViewType* DescriptorView, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        Assert(DescriptorView != nullptr);

        TD3D12DescriptorViewType* CurrentDescriptorView = DescriptorViews[Visibility][ShaderRegister];
        if (DescriptorView != CurrentDescriptorView)
        {
            DescriptorViews[Visibility][ShaderRegister] = DescriptorView;
            Dirty[Visibility] = true;
        
            UInt64& RangeLength = DescriptorRangeLengths[Visibility];
            RangeLength = Math::Max<UInt32>(RangeLength, ShaderRegister + 1);
        }
    }

    void Reset()
    {
        Memory::Memzero(DescriptorViews, sizeof(DescriptorViews));
        Memory::Memzero(Descriptors, sizeof(Descriptors));
        Memory::Memzero(CopyDescriptors, sizeof(CopyDescriptors));
        Memory::Memzero(DescriptorRangeLengths, sizeof(DescriptorRangeLengths));

        for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
        {
            Dirty[i] = true;
        }
    }

    void PrepareForCopy(TD3D12DescriptorViewType* DefaultView)
    {
        TotalNumDescriptors = 0;
        for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
        {
            if (Dirty[i])
            {
                UInt32 NumDescriptors = DescriptorRangeLengths[i];
                UInt32 Offset         = TotalNumDescriptors;
            
                TotalNumDescriptors += NumDescriptors;
                Assert(TotalNumDescriptors <= NUM_DESCRIPTORS);

                for (UInt32 d = 0; d < NumDescriptors; d++)
                {
                    TD3D12DescriptorViewType* View = DescriptorViews[i][d];
                    if (!View)
                    {
                        DescriptorViews[i][d] = View = DefaultView;
                    }
                    
                    CopyDescriptors[Offset + d] = View->GetOfflineHandle();
                }
            }
        }
    }

    void SetGPUHandles(D3D12_GPU_DESCRIPTOR_HANDLE StartHandle, UInt64 DescriptorSize)
    {
        for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
        {
            if (Dirty[i])
            {
                Descriptors[i] = StartHandle;
                StartHandle.ptr += DescriptorRangeLengths[i] * DescriptorSize;

                Dirty[i] = false;
            }
        }
    }

    FORCEINLINE void InvalidateAll()
    {
        for (UInt32 i = 0; i < NUM_VISIBILITIES; i++)
        {
            Dirty[i] = true;
        }
    }

    TD3D12DescriptorViewType*   DescriptorViews[NUM_VISIBILITIES][NUM_DESCRIPTORS];
    D3D12_GPU_DESCRIPTOR_HANDLE Descriptors[NUM_VISIBILITIES];
    D3D12_CPU_DESCRIPTOR_HANDLE CopyDescriptors[NUM_DESCRIPTORS];
    Bool   Dirty[NUM_VISIBILITIES];
    UInt64 DescriptorRangeLengths[NUM_VISIBILITIES];
    UInt32 TotalNumDescriptors;
};

using D3D12ConstantBufferViewCache  = TD3D12DescriptorViewCache<D3D12ConstantBufferView>;
using D3D12ShaderResourceViewCache  = TD3D12DescriptorViewCache<D3D12ShaderResourceView>;
using D3D12UnorderedAccessViewCache = TD3D12DescriptorViewCache<D3D12UnorderedAccessView>;
using D3D12SamplerStateCache        = TD3D12DescriptorViewCache<D3D12SamplerState>;

class D3D12VertexBufferCache
{
public:
    D3D12VertexBufferCache()
        : VertexBuffers()
        , VertexBufferViews()
        , NumVertexBuffers(0)
        , VertexBuffersDirty(false)
        , IndexBuffer(nullptr)
        , IndexBufferView()
        , IndexBufferDirty(false)
    {
        Reset();
    }

    FORCEINLINE void SetVertexBuffer(D3D12VertexBuffer* VertexBuffer, UInt32 Slot)
    {
        Assert(Slot < D3D12_MAX_VERTEX_BUFFER_SLOTS);

        if (VertexBuffers[Slot] != VertexBuffer)
        {
            VertexBuffers[Slot] = VertexBuffer;
            NumVertexBuffers = Math::Max(NumVertexBuffers, Slot + 1);

            VertexBuffersDirty = true;
        }
    }

    FORCEINLINE void SetIndexBuffer(D3D12IndexBuffer* InIndexBuffer)
    {
        if (IndexBuffer != InIndexBuffer)
        {
            IndexBuffer = InIndexBuffer;
            IndexBufferDirty = true;
        }
    }

    FORCEINLINE void CommitState(D3D12CommandListHandle& CmdList)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        if (VertexBuffersDirty)
        {
            for (UInt32 i = 0; i < NumVertexBuffers; i++)
            {
                D3D12VertexBuffer* VertexBuffer = VertexBuffers[i];
                if (!VertexBuffer)
                {
                    VertexBufferViews[i].BufferLocation = 0;
                    VertexBufferViews[i].SizeInBytes    = 0;
                    VertexBufferViews[i].StrideInBytes  = 0;
                }
                else
                {
                    // TODO: Maybe save a ref so that we can ensure that the buffer
                    //       does not get deleted until commandbatch is finished
                    VertexBufferViews[i] = VertexBuffer->GetView();
                }
            }

            DxCmdList->IASetVertexBuffers(0, NumVertexBuffers, VertexBufferViews);
            VertexBuffersDirty = false;
        }

        if (IndexBufferDirty)
        {
            if (!IndexBuffer)
            {
                IndexBufferView.Format         = DXGI_FORMAT_R32_UINT;
                IndexBufferView.BufferLocation = 0;
                IndexBufferView.SizeInBytes    = 0;
            }
            else
            {
                IndexBufferView = IndexBuffer->GetView();
            }

            DxCmdList->IASetIndexBuffer(&IndexBufferView);
            IndexBufferDirty = false;
        }
    }

    FORCEINLINE void Reset()
    {
        Memory::Memzero(VertexBuffers, sizeof(VertexBuffers));
        NumVertexBuffers   = 0;
        VertexBuffersDirty = true;

        IndexBuffer      = nullptr;
        IndexBufferDirty = true;
    }

private:
    D3D12VertexBuffer*       VertexBuffers[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    UInt32 NumVertexBuffers;
    Bool   VertexBuffersDirty;

    D3D12IndexBuffer*       IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    Bool IndexBufferDirty;
};

class D3D12RenderTargetState
{
public:
    D3D12RenderTargetState()
        : RenderTargetViewHandles()
        , NumRenderTargets(0)
        , DepthStencilViewHandle({ 0 })
        , DSVPtr(nullptr)
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
            DSVPtr = &DepthStencilViewHandle;
        }
        else
        {
            DepthStencilViewHandle = { 0 };
            DSVPtr = nullptr;
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
            DxCmdList->OMSetRenderTargets(NumRenderTargets, RenderTargetViewHandles, FALSE, DSVPtr);
            Dirty = false;
        }
    }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandles[D3D12_MAX_RENDER_TARGET_COUNT];
    UInt32 NumRenderTargets;
    D3D12_CPU_DESCRIPTOR_HANDLE  DepthStencilViewHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE* DSVPtr;
    Bool Dirty;
};

class D3D12DescriptorCache : public D3D12DeviceChild
{
public:
    D3D12DescriptorCache(D3D12Device* Device);
    ~D3D12DescriptorCache();

    Bool Init();

    void CommitGraphicsDescriptors(D3D12CommandListHandle& CmdList, class D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature);
    void CommitComputeDescriptors(D3D12CommandListHandle& CmdList, class D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature);

    void Reset();

    FORCEINLINE void SetVertexBuffer(D3D12VertexBuffer* VertexBuffer, UInt32 Slot)
    {
        VertexBufferCache.SetVertexBuffer(VertexBuffer, Slot);
    }

    FORCEINLINE void SetIndexBuffer(D3D12IndexBuffer* IndexBuffer)
    {
        VertexBufferCache.SetIndexBuffer(IndexBuffer);
    }

    FORCEINLINE void SetRenderTargetView(D3D12RenderTargetView* RenderTargetView, UInt32 Slot)
    {
        RenderTargetCache.SetRenderTargetView(RenderTargetView, Slot);
    }

    FORCEINLINE void SetDepthStencilView(D3D12DepthStencilView* DepthStencilView)
    {
        RenderTargetCache.SetDepthStencilView(DepthStencilView);
    }

    FORCEINLINE void SetConstantBufferView(D3D12ConstantBufferView* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullCBV;
        }

        ConstantBufferViewCache.Set(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetShaderResourceView(D3D12ShaderResourceView* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSRV;
        }

        ShaderResourceViewCache.Set(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetUnorderedAccessView(D3D12UnorderedAccessView* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullUAV;
        }

        UnorderedAccessViewCache.Set(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetSamplerState(D3D12SamplerState* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSampler;
        }

        SamplerStateCache.Set(Descriptor, Visibility, ShaderRegister);
    }

private:
    void CopyDescriptors(D3D12OnlineDescriptorHeap* ResourceHeap, D3D12OnlineDescriptorHeap* SamplerHeap);
    
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

    ID3D12DescriptorHeap* PreviousDescriptorHeaps[2] = { nullptr, nullptr };

    UINT RangeSizes[NUM_DESCRIPTORS];
};

class D3D12ShaderConstantsCache
{
public:
    void Set32BitShaderConstants(UInt32* InConstants, UInt32 InNumConstants)
    {
        Memory::Memcpy(Constants, InConstants, sizeof(UInt32) * InNumConstants);
        NumConstants = InNumConstants;
    }

    void CommitGraphics(D3D12CommandListHandle& CmdList, D3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        Int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if (RootIndex >= 0)
        {
            DxCmdList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
        }
    }

    void CommitCompute(D3D12CommandListHandle& CmdList, D3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        Int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if (RootIndex >= 0)
        {
            DxCmdList->SetComputeRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
        }
    }

    void Reset()
    {
        NumConstants = 0;
    }

private:
    UInt32 Constants[D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    UInt32 NumConstants;
};