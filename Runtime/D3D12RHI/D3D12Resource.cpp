#include "D3D12Resource.h"
#include "D3D12Device.h"

FD3D12Resource::FD3D12Resource(FD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Resource(InNativeResource)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InNativeResource->GetDesc())
    , Address(0)
{
}

FD3D12Resource::FD3D12Resource(FD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , HeapType(InHeapType)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InDesc)
    , Address(0)
{
}

bool FD3D12Resource::Initialize(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue)
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    FMemory::Memzero(&HeapProperties);

    HeapProperties.Type                 = HeapType;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc, InitialState, OptimizedClearValue, IID_PPV_ARGS(&Resource));
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
        LOG_ERROR("[FD3D12Resource]: Failed to create committed resource, device ran out of memory");
        return false;
    }
    else
    {
        LOG_ERROR("[FD3D12Resource]: Failed to create committed resource");
        return false;
    }
}

void* FD3D12Resource::MapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range)
{
    void* MappedData = nullptr;

    HRESULT Result = Resource->Map(SubresourceIndex, Range, &MappedData);
    if (FAILED(Result))
    {
        LOG_ERROR("[FD3D12Resource::Map] Failed");
        return nullptr;
    }
    else
    {
        return MappedData;
    }
}

void FD3D12Resource::UnmapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range)
{
    Resource->Unmap(SubresourceIndex, Range);
}

void FD3D12Resource::SetDebugName(const FString& Name)
{
    if (Resource)
    {
        HRESULT Result = Resource->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Size(), Name.GetCString());
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set resource name");
        }

        // Calling SetName as well since NVIDIA Nsight does not recognize the name otherwise
        FStringWide WideName = CharToWide(Name);
        Result = Resource->SetName(WideName.GetCString());
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set resource name");
        }
    }
}

FString FD3D12Resource::GetDebugName() const
{
    if (Resource)
    {
        UINT NameLength;
        
        HRESULT Result = Resource->GetPrivateData(WKPDID_D3DDebugObjectName, &NameLength, nullptr);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to get size of resource name");
        }

        FString NewName;
        NewName.Resize(NameLength);

        Result = Resource->GetPrivateData(WKPDID_D3DDebugObjectName, &NameLength, NewName.Data());
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to get resource name");
        }

        return NewName;
    }

    return "";
}