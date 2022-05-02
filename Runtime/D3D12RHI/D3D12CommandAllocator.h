#pragma once
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12CommandAllocator 

class CD3D12CommandAllocator : public CD3D12DeviceChild
{
public:

    FORCEINLINE CD3D12CommandAllocator(CD3D12Device* InDevice)
        : CD3D12DeviceChild(InDevice)
        , Allocator(nullptr)
    { }

    FORCEINLINE bool Init(D3D12_COMMAND_LIST_TYPE Type)
    {
        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
        if (SUCCEEDED(Result))
        {
            LOG_INFO("[CD3D12CommandAllocator]: Created CommandAllocator");
            return true;
        }
        else
        {
            LOG_ERROR("[CD3D12CommandAllocator]: FAILED to create CommandAllocator");
            return false;
        }
    }

    FORCEINLINE bool Reset()
    {
        HRESULT Result = Allocator->Reset();
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            D3D12DeviceRemovedHandlerRHI(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE void SetName(const String& Name)
    {
        WString WideName = CharToWide(Name);
        Allocator->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12CommandAllocator* GetAllocator() const
    {
        return Allocator.Get();
    }

private:
    TComPtr<ID3D12CommandAllocator> Allocator;
};