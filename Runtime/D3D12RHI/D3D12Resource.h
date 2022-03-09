#pragma once
#include "D3D12DeviceChild.h"

#include "Core/RefCounted.h"
#include "Core/Utilities/StringUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CD3D12Resource> CD3D12ResourceRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Resource

class CD3D12Resource : public CD3D12DeviceObject, public CRefCounted
{
public:

    CD3D12Resource(CD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource);
    CD3D12Resource(CD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType);
    ~CD3D12Resource() = default;

    bool Init(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue);

    bool Map(uint32 SubResource, const D3D12_RANGE* Range, void** OutMappedData)
    void Unmap(uint32 SubResource, const D3D12_RANGE* Range);

    FORCEINLINE void SetName(const String& Name)
    {
        Resource->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), Name.CStr());
    }

    FORCEINLINE ID3D12Resource* GetD3D12Resource() const
    {
        return Resource.Get();
    }

    FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return Address;
    }

    FORCEINLINE D3D12_HEAP_TYPE GetHeapType() const
    {
        return HeapType;
    }

    FORCEINLINE D3D12_RESOURCE_DIMENSION GetDimension() const
    {
        return Desc.Dimension;
    }

    FORCEINLINE D3D12_RESOURCE_STATES GetState() const
    {
        return ResourceState;
    }

    FORCEINLINE const D3D12_RESOURCE_DESC& GetDesc() const
    {
        return Desc;
    }

    FORCEINLINE uint64 GetWidth() const
    {
        return Desc.Width;
    }

private:
    TComPtr<ID3D12Resource>   Resource;

    D3D12_RESOURCE_STATES     ResourceState;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_GPU_VIRTUAL_ADDRESS Address = 0;
};