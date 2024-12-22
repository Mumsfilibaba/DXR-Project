#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"

typedef TSharedRef<class FD3D12Resource> FD3D12ResourceRef;

class FD3D12Resource : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12Resource(FD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource);
    FD3D12Resource(FD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType);

    bool Initialize(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue);
    void* MapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);
    void UnmapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);
    void SetDebugName(const FString& Name);
    FString GetDebugName() const;

    ID3D12Resource* GetD3D12Resource() const 
    { 
        return Resource.Get(); 
    }

    const D3D12_RESOURCE_DESC& GetDesc() const
    {
        return Desc;
    }

    // Texture accessors
    uint64 GetWidth() const { return Desc.Width; }
    uint64 GetHeight() const { return Desc.Height; }
    uint64 GetDepth() const { return Desc.DepthOrArraySize; }
    // Buffer accessors
    uint64 GetSize() const { return Desc.Width; }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return Address; }
    D3D12_HEAP_TYPE GetHeapType() const { return HeapType; }
    D3D12_RESOURCE_STATES GetState() const { return ResourceState; }
    D3D12_RESOURCE_DIMENSION GetDimension() const { return Desc.Dimension; }

private:
    TComPtr<ID3D12Resource>   Resource;
    D3D12_RESOURCE_STATES     ResourceState;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_GPU_VIRTUAL_ADDRESS Address = 0;
};