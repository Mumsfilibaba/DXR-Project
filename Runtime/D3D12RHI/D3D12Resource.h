#pragma once
#include "D3D12DeviceChild.h"

#include "Core/RefCounted.h"
#include "Core/Utilities/StringUtilities.h"

class CD3D12Resource : public CD3D12DeviceChild, public CRefCounted
{
public:
    CD3D12Resource( CD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource );
    CD3D12Resource( CD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType );
    ~CD3D12Resource() = default;

    bool Init( D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue );

    /* Maps the resource to the host */
    void* Map( uint32 SubResource, const D3D12_RANGE* Range );

    /* Unmaps the resource from the host */
    void Unmap( uint32 SubResource, const D3D12_RANGE* Range );

    FORCEINLINE void SetName( const CString& Name )
    {
        WString WideName = CharToWide( Name );
        DxResource->SetName( WideName.CStr() );
    }

    FORCEINLINE ID3D12Resource* GetResource() const
    {
        return DxResource.Get();
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

    // Native resource
    TComPtr<ID3D12Resource> DxResource;

    // Initial resource-state 
    D3D12_RESOURCE_STATES ResourceState;

    // Cached heap-type
    D3D12_HEAP_TYPE HeapType;
    // Cached description
    D3D12_RESOURCE_DESC Desc;
    // Cached GPU address
    D3D12_GPU_VIRTUAL_ADDRESS Address = 0;
};