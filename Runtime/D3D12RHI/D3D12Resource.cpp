#include "D3D12Resource.h"
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12Resource

CD3D12Resource::CD3D12Resource(CD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource)
    : CRefCounted()
    , CD3D12DeviceChild(InDevice)
    , Resource(InNativeResource)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc()
    , Address(0)
{
}

CD3D12Resource::CD3D12Resource(CD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
    : CRefCounted()
    , CD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , HeapType(InHeapType)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InDesc)
    , Address(0)
{
}

bool CD3D12Resource::Init(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue)
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    CMemory::Memzero(&HeapProperties);

    HeapProperties.Type                 = HeapType;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    HRESULT Result = GetDevice()->CreateCommitedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, InitialState, OptimizedClearValue, IID_PPV_ARGS(&Resource));
    if (SUCCEEDED(Result))
    {
        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            Address = Resource->GetGPUVirtualAddress();
        }

        ResourceState = InitialState;
        return true;
    }
    else if (Result == E_OUTOFMEMORY)
    {
        LOG_ERROR("[D3D12Resource]: Failed to create resource since the device ran out of memory");
        return false;
    }
    else
    {
        LOG_ERROR("[D3D12Resource]: Failed to create commited resource");
        return false;
    }
}

void* CD3D12Resource::Map(uint32 SubResource, const D3D12_RANGE* Range)
{
    void* MappedData = nullptr;

    HRESULT Result = Resource->Map(SubResource, Range, &MappedData);
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

void CD3D12Resource::Unmap(uint32 SubResource, const D3D12_RANGE* Range)
{
    Resource->Unmap(SubResource, Range);
}