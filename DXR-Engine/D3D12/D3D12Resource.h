#pragma once
#include "Utilities/StringUtilities.h"

#include "Core/RefCountedObject.h"

#include "D3D12DeviceChild.h"
#include "D3D12Helpers.h"

class D3D12Resource : public D3D12DeviceChild, public RefCountedObject
{
public:
    D3D12Resource(D3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource);
    D3D12Resource(D3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType);
    ~D3D12Resource() = default;

    Bool Init(D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue);

    void* Map(UInt32 Offset, UInt32 Size);
    void  Unmap(UInt32 Offset, UInt32 Size);

    void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        DxResource->SetName(WideName.c_str());
    }

    ID3D12Resource* GetResource() const { return DxResource.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return Address; }

    D3D12_HEAP_TYPE GetHeapType() const { return HeapType; }
    D3D12_RESOURCE_DIMENSION GetDimension() const { return Desc.Dimension; }
    D3D12_RESOURCE_STATES GetState() const { return ResourceState; }
    
    const D3D12_RESOURCE_DESC& GetDesc() const { return Desc; }

private:
    TComPtr<ID3D12Resource>   DxResource;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_STATES     ResourceState;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_GPU_VIRTUAL_ADDRESS Address = 0;
};