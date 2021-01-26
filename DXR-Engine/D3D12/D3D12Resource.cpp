#include "D3D12Resource.h"
#include "D3D12Device.h"

D3D12Resource::D3D12Resource(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
    , NativeResource()
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc()
    , Address(0)
{
}

D3D12Resource::D3D12Resource(D3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource)
    : D3D12DeviceChild(InDevice)
    , NativeResource(InNativeResource)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc()
    , Address(0)
{
}

Void* D3D12Resource::Map(const Range* MappedRange)
{
    Void* MappedData = nullptr;

    HRESULT Result = 0;
    if (MappedRange)
    {
        D3D12_RANGE MapRange = { MappedRange->Offset, MappedRange->Offset + MappedRange->Size };
        Result = NativeResource->Map(0, &MapRange, &MappedData);
    }
    else
    {
        Result = NativeResource->Map(0, nullptr, &MappedData);
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

void D3D12Resource::Unmap(const Range* WrittenRange)
{
    if (WrittenRange)
    {
        D3D12_RANGE WriteRange = { WrittenRange->Offset, WrittenRange->Offset + WrittenRange->Size };
        NativeResource->Unmap(0, &WriteRange);
    }
    else
    {
        NativeResource->Unmap(0, nullptr);
    }
}