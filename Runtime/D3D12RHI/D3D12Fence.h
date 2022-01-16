#pragma once
#include "D3D12Device.h"
#include "D3D12DeviceChild.h"

class CD3D12Fence : public CD3D12DeviceChild
{
public:
    FORCEINLINE CD3D12Fence(CD3D12Device* InDevice)
        : CD3D12DeviceChild(InDevice)
        , Fence(nullptr)
        , Event(0)
    {
    }

    FORCEINLINE ~CD3D12Fence()
    {
        if (Event)
        {
            CloseHandle(Event);
        }
    }

    bool Init(uint64 InitalValue)
    {
        HRESULT Result = GetDevice()->GetDevice()->CreateFence(InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
        if (FAILED(Result))
        {
            LOG_INFO("[D3D12FenceHandle]: FAILED to create Fence");
            return false;
        }

        Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (Event == NULL)
        {
            LOG_ERROR("[D3D12FenceHandle]: FAILED to create Event for Fence");
            return false;
        }

        return true;
    }

    bool WaitForValue(uint64 Value)
    {
        HRESULT Result = Fence->SetEventOnCompletion(Value, Event);
        if (SUCCEEDED(Result))
        {
            WaitForSingleObject(Event, INFINITE);
            return true;
        }
        else
        {
            return false;
        }
    }

    FORCEINLINE bool Signal(uint64 Value)
    {
        HRESULT Result = Fence->Signal(Value);
        return SUCCEEDED(Result);
    }

    FORCEINLINE void SetName(const CString& Name)
    {
        WString WideName = CharToWide(Name);
        Fence->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12Fence* GetFence() const
    {
        return Fence.Get();
    }

private:
    TComPtr<ID3D12Fence> Fence;
    HANDLE Event = 0;
};