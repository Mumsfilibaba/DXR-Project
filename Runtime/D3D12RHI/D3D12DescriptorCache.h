#pragma once
#include "D3D12RootSignature.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CommandList.h"
#include "D3D12RHIBuffer.h"
#include "D3D12RHIViews.h"
#include "D3D12RHISamplerState.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ViewCache

template <typename ViewType, D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint32 kDescriptorTableSize>
class TD3D12ViewCache
{
public:

    static FORCEINLINE D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType()
    {
        return HeapType;
    }

    static FORCEINLINE uint32 GetDescriptorTableSize()
    {
        return kDescriptorTableSize;
    }

    FORCEINLINE TD3D12ViewCache()
        : ResourceViews()
        , HostDescriptors()
        , DeviceDescriptors()
        , CopyDescriptors()
        , bDirty()
    { }

    FORCEINLINE void SetView(ViewType* DescriptorView, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        D3D12_ERROR(DescriptorView != nullptr, "[D3D12]: Trying to bind a ResourceView that was nullptr, check input from DescriptorCache");

        ViewType* CurrentDescriptorView = ResourceViews[Visibility][ShaderRegister];
        if (DescriptorView != CurrentDescriptorView)
        {
            ResourceViews[Visibility][ShaderRegister] = DescriptorView;
            bDirty[Visibility] = true;
        }
    }

    void Reset(ViewType* DefaultView)
    {
        CMemory::Memzero(HostDescriptors  , sizeof(HostDescriptors));
        CMemory::Memzero(DeviceDescriptors, sizeof(DeviceDescriptors));
        CMemory::Memzero(CopyDescriptors  , sizeof(CopyDescriptors));

        for (uint32 Stage = 0; Stage < ShaderVisibility_Count; ++Stage)
        {
            for (uint32 Index = 0; Index < kDescriptorTableSize; ++Index)
            {
                ResourceViews[Stage][Index] = DefaultView;
            }

            bDirty[Stage] = true;
        }
    }

    FORCEINLINE uint32 CountNeededDescriptors() const
    {
        uint32 NumDescriptors = 0;
        for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
        {
            if (bDirty[Stage])
            {
                NumDescriptors += kDescriptorTableSize;
            }
        }

        return NumDescriptors;
    }

    void PrepareForCopy()
    {
        for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
        {
            if (bDirty[Stage])
            {
                for (uint32 Index = 0; Index < kDescriptorTableSize; Index++)
                {
                    ViewType* View = ResourceViews[Stage][Index];
                    Assert(View != nullptr);

                    CopyDescriptors[Stage][Index] = View->GetOfflineHandle();
                }
            }
        }
    }

    void SetAllocatedDescriptorHandles(D3D12_CPU_DESCRIPTOR_HANDLE HostStartHandle, D3D12_GPU_DESCRIPTOR_HANDLE DeviceStartHandle, uint64 IncreamentDescriptorSize)
    {
        for (uint32 Stage = 0; Stage < ShaderVisibility_Count; Stage++)
        {
            if (bDirty[Stage])
            {
                HostDescriptors[Stage] = HostStartHandle;
                HostStartHandle.ptr += (uint64)(kDescriptorTableSize * IncreamentDescriptorSize);

                DeviceDescriptors[Stage] = DeviceStartHandle;
                DeviceStartHandle.ptr += (uint64)(kDescriptorTableSize * IncreamentDescriptorSize);
            }
        }
    }

    FORCEINLINE void InvalidateAll()
    {
        for (uint32 Index = 0; Index < D3D12_CACHED_DESCRIPTORS_NUM_STAGES; Index++)
        {
            bDirty[Index] = true;
        }
    }

    ViewType* ResourceViews[D3D12_CACHED_DESCRIPTORS_NUM_STAGES][kDescriptorTableSize];

    D3D12_CPU_DESCRIPTOR_HANDLE CopyDescriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES][kDescriptorTableSize];

    D3D12_CPU_DESCRIPTOR_HANDLE HostDescriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
    D3D12_GPU_DESCRIPTOR_HANDLE DeviceDescriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
    bool bDirty[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

using CD3D12ConstantBufferViewCache = TD3D12ViewCache<CD3D12RHIConstantBufferView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_CONSTANT_BUFFER_COUNT>;
using CD3D12ShaderResourceViewCache = TD3D12ViewCache<CD3D12RHIShaderResourceView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT>;
using CD3D12UnorderedAccessViewCache = TD3D12ViewCache<CD3D12RHIUnorderedAccessView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT>;
using CD3D12SamplerStateCache = TD3D12ViewCache<CD3D12RHISamplerState, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DEFAULT_SAMPLER_STATE_COUNT>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12VertexBufferCache

class CD3D12VertexBufferCache
{
public:
    FORCEINLINE CD3D12VertexBufferCache()
        : VertexBuffers()
        , VertexBufferViews()
        , NumVertexBuffers(0)
        , bVertexBuffersDirty(false)
        , IndexBuffer(nullptr)
        , IndexBufferView()
        , bIndexBufferDirty(false)
    {
        Reset();
    }

    FORCEINLINE void SetVertexBuffer(CD3D12RHIVertexBuffer* VertexBuffer, uint32 Slot)
    {
        D3D12_ERROR(Slot <= D3D12_MAX_VERTEX_BUFFER_SLOTS, "[D3D12]: Trying to bind a VertexBuffer to a slot (Slot=" + ToString(Slot) + ") higher than the maximum (MaxVertexBufferCount=" + ToString(D3D12_MAX_VERTEX_BUFFER_SLOTS) + ") ");

        if (VertexBuffers[Slot] != VertexBuffer)
        {
            VertexBuffers[Slot] = VertexBuffer;
            NumVertexBuffers = NMath::Max(NumVertexBuffers, Slot + 1);

            bVertexBuffersDirty = true;
        }
    }

    FORCEINLINE void SetIndexBuffer(CD3D12RHIIndexBuffer* InIndexBuffer)
    {
        if (IndexBuffer != InIndexBuffer)
        {
            IndexBuffer = InIndexBuffer;
            bIndexBufferDirty = true;
        }
    }

    void CommitState(CD3D12CommandList& CmdList)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
        if (bVertexBuffersDirty)
        {
            for (uint32 i = 0; i < NumVertexBuffers; i++)
            {
                CD3D12RHIVertexBuffer* VertexBuffer = VertexBuffers[i];
                if (!VertexBuffer)
                {
                    VertexBufferViews[i].BufferLocation = 0;
                    VertexBufferViews[i].SizeInBytes = 0;
                    VertexBufferViews[i].StrideInBytes = 0;
                }
                else
                {
                    // TODO: Maybe save a ref so that we can ensure that the buffer
                    //       does not get deleted until command batch is finished
                    VertexBufferViews[i] = VertexBuffer->GetView();
                }
            }

            DxCmdList->IASetVertexBuffers(0, NumVertexBuffers, VertexBufferViews);
            bVertexBuffersDirty = false;
        }

        if (bIndexBufferDirty)
        {
            if (!IndexBuffer)
            {
                IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
                IndexBufferView.BufferLocation = 0;
                IndexBufferView.SizeInBytes = 0;
            }
            else
            {
                IndexBufferView = IndexBuffer->GetView();
            }

            DxCmdList->IASetIndexBuffer(&IndexBufferView);
            bIndexBufferDirty = false;
        }
    }

    FORCEINLINE void Reset()
    {
        CMemory::Memzero(VertexBuffers, sizeof(VertexBuffers));

        NumVertexBuffers = 0;
        bVertexBuffersDirty = true;

        IndexBuffer = nullptr;
        bIndexBufferDirty = true;
    }

private:
    CD3D12RHIVertexBuffer* VertexBuffers[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    uint32 NumVertexBuffers;
    bool   bVertexBuffersDirty;

    CD3D12RHIIndexBuffer* IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    bool bIndexBufferDirty;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RenderTargetState

class CD3D12RenderTargetState
{
public:
    CD3D12RenderTargetState()
        : RenderTargetViewHandles()
        , NumRenderTargets(0)
        , DepthStencilViewHandle({ 0 })
        , bDirty(false)
    {
        Reset();
    }

    FORCEINLINE void SetRenderTargetView(CD3D12RHIRenderTargetView* RenderTargetView, uint32 Slot)
    {
        D3D12_ERROR(Slot <= D3D12_MAX_RENDER_TARGET_COUNT, "[D3D12]: Trying to bind a RenderTarget to a slot (Slot=" + ToString(Slot) + ") higher than the maximum (MaxRenderTargetCount=" + ToString(D3D12_MAX_RENDER_TARGET_COUNT) + ") ");

        if (RenderTargetView)
        {
            RenderTargetViewHandles[Slot] = RenderTargetView->GetOfflineHandle();
        }
        else
        {
            RenderTargetViewHandles[Slot] = { 0 };
        }

        NumRenderTargets = NMath::Max(NumRenderTargets, Slot + 1);
        bDirty = true;
    }

    FORCEINLINE void SetDepthStencilView(CD3D12RHIDepthStencilView* DepthStencilView)
    {
        if (DepthStencilView)
        {
            DepthStencilViewHandle = DepthStencilView->GetOfflineHandle();
        }
        else
        {
            DepthStencilViewHandle = { 0 };
        }

        bDirty = true;
    }

    FORCEINLINE void Reset()
    {
        CMemory::Memzero(RenderTargetViewHandles, sizeof(RenderTargetViewHandles));
        DepthStencilViewHandle = { 0 };
        NumRenderTargets = 0;
    }

    FORCEINLINE void CommitState(CD3D12CommandList& CmdList)
    {
        if (bDirty)
        {
            ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

            D3D12_CPU_DESCRIPTOR_HANDLE* DepthStencil = nullptr;
            if (DepthStencilViewHandle.ptr)
            {
                DepthStencil = &DepthStencilViewHandle;
            }

            DxCmdList->OMSetRenderTargets(NumRenderTargets, RenderTargetViewHandles, false, DepthStencil);
            bDirty = false;
        }
    }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandles[D3D12_MAX_RENDER_TARGET_COUNT];
    uint32 NumRenderTargets;

    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilViewHandle;

    bool bDirty;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12DescriptorCache

class CD3D12DescriptorCache : public CD3D12DeviceChild
{
public:
    CD3D12DescriptorCache(CD3D12Device* Device);
    ~CD3D12DescriptorCache();

    bool Init();

    void CommitGraphicsDescriptors(CD3D12CommandList& CmdList, class CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature);
    void CommitComputeDescriptors(CD3D12CommandList& CmdList, class CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature);

    void Reset();

    FORCEINLINE void SetVertexBuffer(CD3D12RHIVertexBuffer* VertexBuffer, uint32 Slot)
    {
        VertexBufferCache.SetVertexBuffer(VertexBuffer, Slot);
    }

    FORCEINLINE void SetIndexBuffer(CD3D12RHIIndexBuffer* IndexBuffer)
    {
        VertexBufferCache.SetIndexBuffer(IndexBuffer);
    }

    FORCEINLINE void SetRenderTargetView(CD3D12RHIRenderTargetView* RenderTargetView, uint32 Slot)
    {
        RenderTargetCache.SetRenderTargetView(RenderTargetView, Slot);
    }

    FORCEINLINE void SetDepthStencilView(CD3D12RHIDepthStencilView* DepthStencilView)
    {
        RenderTargetCache.SetDepthStencilView(DepthStencilView);
    }

    FORCEINLINE void SetConstantBufferView(CD3D12RHIConstantBufferView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullCBV;
        }

        ConstantBufferViewCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetShaderResourceView(CD3D12RHIShaderResourceView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSRV;
        }

        ShaderResourceViewCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetUnorderedAccessView(CD3D12RHIUnorderedAccessView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullUAV;
        }

        UnorderedAccessViewCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetSamplerState(CD3D12RHISamplerState* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSampler;
        }

        SamplerStateCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

private:
    void AllocateDescriptorsAndSetHeaps(ID3D12GraphicsCommandList* CmdList, CD3D12OnlineDescriptorHeap* ResourceHeap, CD3D12OnlineDescriptorHeap* SamplerHeap);

    template<typename TResourveViewCache>
    void CopyAndBindComputeDescriptors(ID3D12Device* DxDevice, ID3D12GraphicsCommandList* DxCmdList, TResourveViewCache& ResourceViewCache, int32 ParameterIndex)
    {
        const EShaderVisibility ShaderVisibility = ShaderVisibility_All;
        if (ParameterIndex >= 0 && ResourceViewCache.bDirty[ShaderVisibility])
        {
            const UINT DestRangeSize = TResourveViewCache::GetDescriptorTableSize();
            const UINT NumSrcRanges = TResourveViewCache::GetDescriptorTableSize();

            const D3D12_CPU_DESCRIPTOR_HANDLE* SrcHostStarts = ResourceViewCache.CopyDescriptors[ShaderVisibility];

            D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = ResourceViewCache.HostDescriptors[ShaderVisibility];
            DxDevice->CopyDescriptors(1, &HostHandle, &DestRangeSize, NumSrcRanges, SrcHostStarts, RangeSizes, TResourveViewCache::GetDescriptorHeapType());

            D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = ResourceViewCache.DeviceDescriptors[ShaderVisibility];
            DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, DeviceHandle);

            // When the descriptors are copied and bound, then the descriptors are not dirty anymore
            ResourceViewCache.bDirty[ShaderVisibility] = false;
        }
    }

    template<typename TResourveViewCache>
    void CopyAndBindGraphicsDescriptors(ID3D12Device* DxDevice, ID3D12GraphicsCommandList* DxCmdList, TResourveViewCache& ResourceViewCache, int32 ParameterIndex, EShaderVisibility ShaderVisibility)
    {
        if (ParameterIndex >= 0 && ResourceViewCache.bDirty[ShaderVisibility])
        {
            const UINT DestRangeSize = TResourveViewCache::GetDescriptorTableSize();
            const UINT NumSrcRanges = TResourveViewCache::GetDescriptorTableSize();

            const D3D12_CPU_DESCRIPTOR_HANDLE* SrcHostStarts = ResourceViewCache.CopyDescriptors[ShaderVisibility];

            D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = ResourceViewCache.HostDescriptors[ShaderVisibility];
            DxDevice->CopyDescriptors(1, &HostHandle, &DestRangeSize, NumSrcRanges, SrcHostStarts, RangeSizes, TResourveViewCache::GetDescriptorHeapType());

            D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = ResourceViewCache.DeviceDescriptors[ShaderVisibility];
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, DeviceHandle);

            // When the descriptors are copied and bound, then the descriptors are not dirty anymore
            ResourceViewCache.bDirty[ShaderVisibility] = false;
        }
    }

    CD3D12RHIConstantBufferView* NullCBV = nullptr;
    CD3D12RHIShaderResourceView* NullSRV = nullptr;
    CD3D12RHIUnorderedAccessView* NullUAV = nullptr;
    CD3D12RHISamplerState* NullSampler = nullptr;

    CD3D12VertexBufferCache        VertexBufferCache;
    CD3D12RenderTargetState        RenderTargetCache;
    CD3D12ShaderResourceViewCache  ShaderResourceViewCache;
    CD3D12UnorderedAccessViewCache UnorderedAccessViewCache;
    CD3D12ConstantBufferViewCache  ConstantBufferViewCache;
    CD3D12SamplerStateCache        SamplerStateCache;

    ID3D12DescriptorHeap* PreviousDescriptorHeaps[2] = { nullptr, nullptr };

    UINT RangeSizes[D3D12_CACHED_DESCRIPTORS_COUNT];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12ShaderConstantsCache

class CD3D12ShaderConstantsCache
{
public:

    FORCEINLINE CD3D12ShaderConstantsCache()
        : Constants()
        , NumConstants()
    {
        Reset();
    }

    FORCEINLINE void Set32BitShaderConstants(const uint32* InConstants, uint32 InNumConstants)
    {
        D3D12_ERROR(InNumConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT, "[D3D12]: Trying to set a number of shader-constants (NumConstants=" + ToString(InNumConstants) + ") higher than the maximum (MaxShaderConstants=" + ToString(D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT) + ") ");

        CMemory::Memcpy(Constants, InConstants, sizeof(uint32) * InNumConstants);
        NumConstants = InNumConstants;
    }

    FORCEINLINE void CommitGraphics(CD3D12CommandList& CmdList, CD3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if (RootIndex >= 0)
        {
            DxCmdList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
        }
    }

    FORCEINLINE void CommitCompute(CD3D12CommandList& CmdList, CD3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if (RootIndex >= 0)
        {
            DxCmdList->SetComputeRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
        }
    }

    FORCEINLINE void Reset()
    {
        CMemory::Memzero(Constants, sizeof(Constants));
        NumConstants = 0;
    }

private:
    uint32 Constants[D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    uint32 NumConstants;
};