#pragma once
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12CommandAllocator 

class CD3D12CommandAllocator : public CD3D12DeviceObject
{
public:

    FORCEINLINE CD3D12CommandAllocator(CD3D12Device* InDevice)
        : CD3D12DeviceObject(InDevice)
        , Allocator(nullptr)
    {
    }

    FORCEINLINE bool Initialize(D3D12_COMMAND_LIST_TYPE Type)
    {
        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
        if (SUCCEEDED(Result))
        {
            D3D12_INFO("Created CommandAllocator");
            return true;
        }
        else
        {
            D3D12_ERROR_ALWAYS("FAILED to create CommandAllocator");
            return false;
        }
    }

    FORCEINLINE bool Reset()
    {
        HRESULT Result = Allocator->Reset();
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12RHIDeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE void SetName(const String& Name)
    {
        Allocator->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), Name.CStr());
    }

    FORCEINLINE ID3D12CommandAllocator* GetAllocator() const
    {
        return Allocator.Get();
    }

private:
    TComPtr<ID3D12CommandAllocator> Allocator;
};