#pragma once
#include "Utilities/StringUtilities.h"

#include "RenderLayer/Resources.h"

#include "D3D12DeviceChild.h"

class D3D12Resource : public D3D12DeviceChild
{
    friend class D3D12RenderLayer;

public:
    D3D12Resource(D3D12Device* InDevice);
    D3D12Resource(D3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource);
    virtual ~D3D12Resource() = default;

    void* Map(UInt32 Offset, UInt32 Size);
    void  Unmap(UInt32 Offset, UInt32 Size);

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        NativeResource->SetName(WideName.c_str());

        DebugName = Name;
    }

    FORCEINLINE ID3D12Resource* GetNativeResource() const
    {
        return NativeResource.Get();
    }

    FORCEINLINE D3D12_RESOURCE_DIMENSION GetResourceDimension() const
    {
        return Desc.Dimension;
    }

    FORCEINLINE D3D12_HEAP_TYPE GetHeapType() const
    {
        return HeapType;
    }

    FORCEINLINE D3D12_RESOURCE_STATES GetResourceState() const
    {
        return ResourceState;
    }

    FORCEINLINE const D3D12_RESOURCE_DESC& GetDesc() const
    {
        return Desc;
    }

    FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
    {
        return Address;
    }

protected:
    TComPtr<ID3D12Resource> NativeResource;
    std::string DebugName;

    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_STATES     ResourceState;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_GPU_VIRTUAL_ADDRESS Address;
};