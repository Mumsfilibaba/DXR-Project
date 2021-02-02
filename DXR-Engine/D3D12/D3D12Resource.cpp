#include "D3D12Resource.h"
#include "D3D12Device.h"

D3D12Resource::D3D12Resource(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
    , DxResource()
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc()
    , Address(0)
{
}

D3D12Resource::D3D12Resource(D3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource)
    : D3D12DeviceChild(InDevice)
    , DxResource(InNativeResource)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc()
    , Address(0)
{
}

D3D12Resource::D3D12Resource(D3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
    : D3D12DeviceChild(InDevice)
    , DxResource(nullptr)
    , HeapType(InHeapType)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InDesc)
    , Address(0)
{
}

Bool D3D12Resource::Init(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue)
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    Memory::Memzero(&HeapProperties);

    HeapProperties.Type                 = HeapType;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    HRESULT Result = Device->CreateCommitedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, InitialState, OptimizedClearValue, IID_PPV_ARGS(&DxResource));
    if (SUCCEEDED(Result))
    {
        Address       = DxResource->GetGPUVirtualAddress();
        ResourceState = InitialState;
        return true;
    }
    else
    {
        LOG_ERROR("[D3D12Resource]: Failed to create commited resource");
        return false;
    }
}

void* D3D12Resource::Map(UInt32 Offset, UInt32 Size)
{
    void* MappedData = nullptr;

    HRESULT Result = 0;
    if (Offset != 0 && Size != 0)
    {
        D3D12_RANGE MapRange = { Offset,Offset + Size };
        Result = DxResource->Map(0, &MapRange, &MappedData);
    }
    else
    {
        Result = DxResource->Map(0, nullptr, &MappedData);
    }

    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12Resource::Map] Failed");
        return nullptr;
    }
    else
    {
        return MappedData;
    }
}

void D3D12Resource::Unmap(UInt32 Offset, UInt32 Size)
{
    if (Offset != 0 && Size != 0)
    {
        D3D12_RANGE WriteRange = { Offset,Offset + Size };
        DxResource->Unmap(0, &WriteRange);
    }
    else
    {
        DxResource->Unmap(0, nullptr);
    }
}