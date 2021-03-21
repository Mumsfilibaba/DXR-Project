#pragma once
#include "D3D12Device.h"

class D3D12CommandAllocatorHandle : public D3D12DeviceChild
{
public:
    D3D12CommandAllocatorHandle(D3D12Device* InDevice)
        : D3D12DeviceChild(InDevice)
        , Allocator(nullptr)
    {
    }

    FORCEINLINE bool Init(D3D12_COMMAND_LIST_TYPE Type)
    {
        HRESULT Result = GetDevice()->GetDevice()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
        if (SUCCEEDED(Result))
        {
            LOG_INFO("[D3D12Device]: Created CommandAllocator");
            return true;
        }
        else
        {
            LOG_ERROR("[D3D12Device]: FAILED to create CommandAllocator");
            return false;
        }
    }

    FORCEINLINE bool Reset()
    {
        HRESULT Result = Allocator->Reset();
        if (Result == DXGI_ERROR_DEVICE_REMOVED)
        {
            DeviceRemovedHandler(GetDevice());
        }

        return SUCCEEDED(Result);
    }

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        Allocator->SetName(WideName.c_str());
    }

    FORCEINLINE ID3D12CommandAllocator* GetAllocator() const
    {
        return Allocator.Get();
    }

private:
    TComPtr<ID3D12CommandAllocator> Allocator;
};