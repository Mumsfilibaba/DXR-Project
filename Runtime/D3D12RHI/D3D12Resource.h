#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"

#include "Core/Utilities/StringUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class FD3D12Resource> FD3D12ResourceRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Resource

class FD3D12Resource 
    : public FD3D12DeviceChild
    , public FD3D12RefCounted
{
public:
    FD3D12Resource(FD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource);
    FD3D12Resource(FD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType);
    ~FD3D12Resource() = default;

    bool  Initialize(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue);

    void* MapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);
    void  UnmapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);

    FORCEINLINE void SetName(const FString& Name)
    {
        FWString WideName = CharToWide(Name);
        Resource->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12Resource*            GetD3D12Resource() const { return Resource.Get(); }

    FORCEINLINE const D3D12_RESOURCE_DESC& GetDesc() const { return Desc; }

    FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS  GetGPUVirtualAddress() const { return Address; }
    FORCEINLINE D3D12_HEAP_TYPE            GetHeapType()          const { return HeapType; }
    FORCEINLINE D3D12_RESOURCE_STATES      GetState()             const { return ResourceState; }
    FORCEINLINE D3D12_RESOURCE_DIMENSION   GetDimension()         const { return Desc.Dimension; }

    FORCEINLINE uint64                     GetWidth()  const { return Desc.Width; }
    FORCEINLINE uint64                     GetHeight() const { return Desc.Height; }
    FORCEINLINE uint64                     GetDepth()  const { return Desc.DepthOrArraySize; }

private:
    TComPtr<ID3D12Resource>   Resource;

    D3D12_RESOURCE_STATES     ResourceState;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_GPU_VIRTUAL_ADDRESS Address = 0;
};