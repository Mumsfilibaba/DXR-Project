#pragma once
#include "D3D12RootSignature.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CommandList.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"
#include "D3D12SamplerState.h"

class FD3D12CommandBatch;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12ViewCache

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
        D3D12_ERROR_COND(DescriptorView != nullptr, "Trying to bind a ResourceView that was nullptr, check input from DescriptorCache");

        ViewType* CurrentDescriptorView = ResourceViews[Visibility][ShaderRegister];
        if (DescriptorView != CurrentDescriptorView)
        {
            ResourceViews[Visibility][ShaderRegister] = DescriptorView;
            bDirty[Visibility] = true;
        }
    }

    void Reset(ViewType* DefaultView)
    {
        FMemory::Memzero(HostDescriptors  , sizeof(HostDescriptors));
        FMemory::Memzero(DeviceDescriptors, sizeof(DeviceDescriptors));
        FMemory::Memzero(CopyDescriptors  , sizeof(CopyDescriptors));

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
                    Check(View != nullptr);

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

using FD3D12ConstantBufferViewCache  = TD3D12ViewCache<FD3D12ConstantBufferView , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_CONSTANT_BUFFER_COUNT>;
using FD3D12ShaderResourceViewCache  = TD3D12ViewCache<FD3D12ShaderResourceView , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT>;
using FD3D12UnorderedAccessViewCache = TD3D12ViewCache<FD3D12UnorderedAccessView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT>;
using FD3D12SamplerStateCache        = TD3D12ViewCache<FD3D12SamplerState       , D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER    , D3D12_DEFAULT_SAMPLER_STATE_COUNT>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VertexBufferCache

class FD3D12VertexBufferCache
{
public:
    
    FD3D12VertexBufferCache()
        : VertexBuffers()
        , NumVertexBuffers(0)
        , bVertexBuffersDirty(false)
        , IndexBuffer(nullptr)
        , bIndexBufferDirty(false)
    {
        Reset();
    }

    void CommitState(FD3D12CommandList& CmdList, FD3D12CommandBatch* CmdBatch);

    FORCEINLINE void SetVertexBuffer(FD3D12VertexBuffer* VertexBuffer, uint32 Slot)
    {
        D3D12_ERROR_COND(Slot <= D3D12_MAX_VERTEX_BUFFER_SLOTS, "Trying to bind a VertexBuffer to a slot (Slot=%u) higher than the maximum (MaxVertexBufferCount=%u)", Slot, D3D12_MAX_VERTEX_BUFFER_SLOTS);
                        
        if (VertexBuffers[Slot] != VertexBuffer)
        {
            VertexBuffers[Slot] = VertexBuffer;
            NumVertexBuffers    = NMath::Max(NumVertexBuffers, Slot + 1);

            bVertexBuffersDirty = true;
        }
    }

    FORCEINLINE void SetIndexBuffer(FD3D12IndexBuffer* InIndexBuffer)
    {
        if (IndexBuffer != InIndexBuffer)
        {
            IndexBuffer = InIndexBuffer;
            bIndexBufferDirty = true;
        }
    }

    FORCEINLINE void Reset()
    {
        VertexBuffers.Memzero();

        NumVertexBuffers    = 0;
        bVertexBuffersDirty = true;

        IndexBuffer       = nullptr;
        bIndexBufferDirty = true;
    }

private:
    TStaticArray<FD3D12VertexBuffer*, D3D12_MAX_VERTEX_BUFFER_SLOTS> VertexBuffers;
    uint32 NumVertexBuffers;

    FD3D12IndexBuffer* IndexBuffer;

    bool bVertexBuffersDirty;
    bool bIndexBufferDirty;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RenderTargetState

class FD3D12RenderTargetState
{
public:
    FD3D12RenderTargetState()
        : RenderTargetViewHandles()
        , NumRenderTargets(0)
        , DepthStencilViewHandle({ 0 })
        , bDirty(false)
    {
        Reset();
    }

    FORCEINLINE void SetRenderTargetView(FD3D12RenderTargetView* RenderTargetView, uint32 Slot)
    {
        D3D12_ERROR_COND(Slot <= D3D12_MAX_RENDER_TARGET_COUNT, "Trying to bind a RenderTarget to a slot (Slot=%u) higher than the maximum (MaxRenderTargetCount=%u)", Slot, D3D12_MAX_RENDER_TARGET_COUNT);

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

    FORCEINLINE void SetDepthStencilView(FD3D12DepthStencilView* DepthStencilView)
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

        FMemory::Memzero(RenderTargetViewHandles, sizeof(RenderTargetViewHandles));
        DepthStencilViewHandle = { 0 };
        NumRenderTargets       = 0;
    }

    FORCEINLINE void CommitState(FD3D12CommandList& CmdList)
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
// FD3D12DescriptorCache

class FD3D12DescriptorCache : public FD3D12DeviceChild
{
public:
    FD3D12DescriptorCache(FD3D12Device* Device);
    ~FD3D12DescriptorCache();

    bool Init();

    void CommitGraphicsDescriptors(FD3D12CommandList& CmdList, FD3D12CommandBatch* CmdBatch, FD3D12RootSignature* RootSignature);
    void CommitComputeDescriptors(FD3D12CommandList& CmdList, FD3D12CommandBatch* CmdBatch, FD3D12RootSignature* RootSignature);

    void Reset();

    FORCEINLINE void SetVertexBuffer(FD3D12VertexBuffer* VertexBuffer, uint32 Slot)
    {
        VertexBufferCache.SetVertexBuffer(VertexBuffer, Slot);
    }

    FORCEINLINE void SetIndexBuffer(FD3D12IndexBuffer* IndexBuffer)
    {
        VertexBufferCache.SetIndexBuffer(IndexBuffer);
    }

    FORCEINLINE void SetRenderTargetView(FD3D12RenderTargetView* RenderTargetView, uint32 Slot)
    {
        RenderTargetCache.SetRenderTargetView(RenderTargetView, Slot);
    }

    FORCEINLINE void SetDepthStencilView(FD3D12DepthStencilView* DepthStencilView)
    {
        RenderTargetCache.SetDepthStencilView(DepthStencilView);
    }

    FORCEINLINE void SetConstantBufferView(FD3D12ConstantBufferView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullCBV;
        }

        ConstantBufferViewCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetShaderResourceView(FD3D12ShaderResourceView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSRV;
        }

        ShaderResourceViewCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetUnorderedAccessView(FD3D12UnorderedAccessView* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullUAV;
        }

        UnorderedAccessViewCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

    FORCEINLINE void SetSamplerState(FD3D12SamplerState* Descriptor, EShaderVisibility Visibility, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSampler;
        }

        SamplerStateCache.SetView(Descriptor, Visibility, ShaderRegister);
    }

private:
    void AllocateDescriptorsAndSetHeaps(ID3D12GraphicsCommandList* CmdList, FD3D12OnlineDescriptorHeap* ResourceHeap, FD3D12OnlineDescriptorHeap* SamplerHeap);

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

    FD3D12ConstantBufferView*      NullCBV     = nullptr;
    FD3D12ShaderResourceView*      NullSRV     = nullptr;
    FD3D12UnorderedAccessView*     NullUAV     = nullptr;
    FD3D12SamplerState*            NullSampler = nullptr;

    FD3D12VertexBufferCache        VertexBufferCache;
    FD3D12RenderTargetState        RenderTargetCache;
    FD3D12ShaderResourceViewCache  ShaderResourceViewCache;
    FD3D12UnorderedAccessViewCache UnorderedAccessViewCache;
    FD3D12ConstantBufferViewCache  ConstantBufferViewCache;
    FD3D12SamplerStateCache        SamplerStateCache;

    ID3D12DescriptorHeap*          PreviousDescriptorHeaps[2] = { nullptr, nullptr };

    UINT RangeSizes[D3D12_CACHED_DESCRIPTORS_COUNT];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ShaderConstantsCache

class FD3D12ShaderConstantsCache
{
public:

    FD3D12ShaderConstantsCache()
        : Constants()
        , NumConstants()
        , bIsDirty(false)
    {
        Reset();
    }

    FORCEINLINE void Set32BitShaderConstants(const uint32* InConstants, uint32 InNumConstants)
    {
        D3D12_ERROR_COND( InNumConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT
                        , "Trying to set a number of shader-constants (NumConstants=%u) higher than the maximum (MaxShaderConstants=%u)"
                        , InNumConstants
                        , D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT);

        FMemory::Memcpy(Constants, InConstants, sizeof(uint32) * InNumConstants);
        NumConstants = InNumConstants;

        bIsDirty = true;
    }

    FORCEINLINE void CommitGraphics(FD3D12CommandList& CmdList, FD3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ((RootIndex >= 0) && bIsDirty)
        {
            DxCmdList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
            bIsDirty = false;
        }
    }

    FORCEINLINE void CommitCompute(FD3D12CommandList& CmdList, FD3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ((RootIndex >= 0) && bIsDirty)
        {
            DxCmdList->SetComputeRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
            bIsDirty = false;
        }
    }

    FORCEINLINE void Reset()
    {
        FMemory::Memzero(Constants, sizeof(Constants));
        NumConstants = 0;

        bIsDirty = true;
    }

private:
    uint32 Constants[D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    uint32 NumConstants;

    bool   bIsDirty;
};