#pragma once
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandAllocator 

class FD3D12CommandAllocator : public FD3D12DeviceChild
{
public:

    FORCEINLINE FD3D12CommandAllocator(FD3D12Device* InDevice)
        : FD3D12DeviceChild(InDevice)
        , Allocator(nullptr)
    { }

    FORCEINLINE bool Init(D3D12_COMMAND_LIST_TYPE Type)
    {
        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
        if (SUCCEEDED(Result))
        {
            D3D12_INFO("[FD3D12CommandAllocator]: Created CommandAllocator");
            return true;
        }
        else
        {
            D3D12_ERROR("[FD3D12CommandAllocator]: FAILED to create CommandAllocator");
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

    FORCEINLINE void SetName(const FString& Name)
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