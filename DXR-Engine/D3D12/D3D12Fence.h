#pragma once
#include "D3D12DeviceChild.h"

class D3D12FenceHandle : public D3D12DeviceChild
{
public:
    D3D12FenceHandle(D3D12Device* InDevice)
        : D3D12DeviceChild(InDevice)
        , Fence(nullptr)
        , Event(0)
    {
    }

    ~D3D12FenceHandle()
    {
        ::CloseHandle(Event);
    }

    FORCEINLINE Bool Init(UInt64 InitalValue)
    {
        HRESULT Result = Device->GetDevice()->CreateFence(InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
        if (SUCCEEDED(Result))
        {
            Event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (Event == NULL)
            {
                LOG_ERROR("[D3D12Device]: FAILED to create Event for Fence");
                return false;
            }
            else
            {
                LOG_INFO("[D3D12Device]: Created Fence");
                return true;
            }
        }
        else
        {
            LOG_INFO("[D3D12Device]: FAILED to create Fence");
            return false;
        }
    }

    FORCEINLINE Bool WaitForValue(UInt64 Value)
    {
        HRESULT Result = Fence->SetEventOnCompletion(Value, Event);
        if (SUCCEEDED(Result))
        {
            ::WaitForSingleObjectEx(Event, INFINITE, FALSE);
            return true;
        }
        else
        {
            return false;
        }
    }

    FORCEINLINE Bool Signal(UInt64 Value)
    {
        HRESULT Result = Fence->Signal(Value);
        return SUCCEEDED(Result);
    }

    FORCEINLINE void SetName(const std::string& Name)
    {
        std::wstring WideName = ConvertToWide(Name);
        Fence->SetName(WideName.c_str());
    }

    FORCEINLINE ID3D12Fence* GetFence() const
    {
        return Fence.Get();
    }

private:
    TComPtr<ID3D12Fence> Fence;
    HANDLE Event = 0;
};