#include "D3D12Resource.h"
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Resource

CD3D12ResourceRef CD3D12Resource::CreateResource(CD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource)
{
    CD3D12ResourceRef NewResource = dbg_new CD3D12Resource(InDevice);
    if (NewResource && NewResource->Initialize(InNativeResource))
    {
        return NewResource;
    }

    return nullptr;
}

CD3D12ResourceRef CD3D12Resource::CreateResource(CD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue)
{
    CD3D12ResourceRef NewResource = dbg_new CD3D12Resource(InDevice, InDesc, InHeapType);
    if (NewResource && NewResource->Initialize(InitialState, OptimizedClearValue))
    {
        return NewResource;
    }

    return nullptr;
}

CD3D12Resource::CD3D12Resource(CD3D12Device* InDevice)
    : CRefCounted()
    , CD3D12DeviceObject(InDevice)
    , Resource(nullptr)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc()
    , Address(0)
{
}

CD3D12Resource::CD3D12Resource(CD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
    : CRefCounted()
    , CD3D12DeviceObject(InDevice)
    , Resource(nullptr)
    , HeapType(InHeapType)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InDesc)
    , Address(0)
{
}

bool CD3D12Resource::Initialize(const TComPtr<ID3D12Resource>& InNativeResource)
{
    if (InNativeResource)
    {
        Resource = InNativeResource;

        D3D12_HEAP_FLAGS      HeapFlags;
        D3D12_HEAP_PROPERTIES HeapProperties;

        HRESULT Result = Resource->GetHeapProperties(&HeapProperties, &HeapFlags);
        D3D12_ERROR(SUCCEEDED(Result), "GetHeapProperties failed");

        Desc     = Resource->GetDesc();
        HeapType = HeapProperties.Type;

        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            Address = Resource->GetGPUVirtualAddress();
        }
    }

    return true;
}

bool CD3D12Resource::Initialize(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue)
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
        D3D12_ERROR_ALWAYS("Failed to create resource since the device ran out of memory");
        return false;
    }
    else
    {
        D3D12_ERROR_ALWAYS("Failed to create commited resource");
        return false;
    }
}

bool CD3D12Resource::MapResource(uint32 Subresource, const D3D12_RANGE* ReadRange, void** OutMappedData)
{
    HRESULT Result = Resource->Map(Subresource, ReadRange, OutMappedData);
    if (FAILED(Result))
    {
        *OutMappedData = nullptr;
        D3D12_ERROR_ALWAYS("Map Failed");
        return false;
    }
    else
    {
        return true;
    }
}

void CD3D12Resource::UnmapResource(uint32 Subresource, const D3D12_RANGE* WrittenRange)
{
    Resource->Unmap(Subresource, WrittenRange);
}