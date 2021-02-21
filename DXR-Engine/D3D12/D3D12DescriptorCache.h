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
            RangeLength = Math::Max<UInt32>(RangeLength, ShaderRegister);

            Dirty = true;
        }
    }

    void Reset()
    {
        Memory::Memzero(DirtyMasks, sizeof(DirtyMasks));
        Memory::Memzero(Descriptors, sizeof(Descriptors));
        Memory::Memzero(DescriptorRangeLengths, sizeof(DescriptorRangeLengths));
    }

    Bool IsDirty() const { return Dirty; }

    // NOTE: This is ALOT of memory, we should probably refactor this, maybe a class that 
    //       handles descriptorbindings. class ResourceTable? That can be stored outside the renderlayer
    //       problem is how we deal with ClearUnorderedAccessView, should we change heap when this happens?
    //       However this would better support unbound resources.
    UInt64 DirtyMasks[NUM_VISIBILITIES][NUM_DIRTY_MASKS];
    D3D12_CPU_DESCRIPTOR_HANDLE Descriptors[NUM_VISIBILITIES][NUM_DESCRIPTORS];
    UInt64 DescriptorRangeLengths[NUM_VISIBILITIES];
    Bool Dirty;
};

using D3D12ConstantBufferViewCache  = TD3D12DescriptorCache<D3D12ConstantBufferView>;
using D3D12ShaderResourceViewCache  = TD3D12DescriptorCache<D3D12ShaderResourceView>;
using D3D12UnorderedAccessViewCache = TD3D12DescriptorCache<D3D12UnorderedAccessView>;
using D3D12SamplerStateCache        = TD3D12DescriptorCache<D3D12SamplerState>;

class D3D12DescriptorCache : public D3D12DeviceChild
{
public:
    D3D12DescriptorCache(D3D12Device* Device);
    ~D3D12DescriptorCache() = default;

    Bool Init();

    void SetUnorderedAccessView(D3D12UnorderedAccessView* Descriptor, EShaderVisibility Visibility, UInt32 ShaderRegister)
    {
        
    }

    void CommitGraphicsDescriptorTables(D3D12CommandListHandle& CmdList, D3D12RootSignature& RootSignature);
    void CommitComputeDescriptorTables(D3D12CommandListHandle& CmdList, D3D12RootSignature& RootSignature);

    void Reset();

private:
    D3D12ShaderResourceViewCache  ShaderResourceViewCache;
    D3D12UnorderedAccessViewCache UnorderedAccessViewCache;
    D3D12ConstantBufferViewCache  ConstantBufferViewCache;
    D3D12SamplerStateCache        SamplerStateCache;
};