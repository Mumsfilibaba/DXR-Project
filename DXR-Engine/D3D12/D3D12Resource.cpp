#include "D3D12Resource.h"
#include "D3D12Device.h"

D3D12Resource::D3D12Resource(D3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource)
    : RefCountedObject()
    , D3D12DeviceChild(InDevice)
    , DxResource(InNativeResource)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc()
    , Address(0)
{
}

D3D12Resource::D3D12Resource(D3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
    : RefCountedObject()
    , D3D12DeviceChild(InDevice)
    , DxResource(nullptr)
    , HeapType(InHeapType)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InDesc)
    , Address(0)
{
}

bool D3D12Resource::Init(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue)
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    Memory::Memzero(&HeapProperties);

    HeapProperties.Type                 = HeapType;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    HRESULT Result = GetDevice()->CreateCommitedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, InitialState, OptimizedClearValue, IID_PPV_ARGS(&DxResource));
    if (SUCCEEDED(Result))
    {
        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            Address = DxResource->GetGPUVirtualAddress();
        }
        
        ResourceState = InitialState;
        return true;
    }
    else
    {
        LOG_ERROR("[D3D12Resource]: Failed to create commited resource");
        return false;
    }
}

void* D3D12Resource::Map(uint32 SubResource, const D3D12_RANGE* Range)
{
    void* MappedData = nullptr;

    HRESULT Result = DxResource->Map(SubResource, Range, &MappedData);
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

void D3D12Resource::Unmap(uint32 SubResource, const D3D12_RANGE* Range)
{
    DxResource->Unmap(SubResource, Range);
}