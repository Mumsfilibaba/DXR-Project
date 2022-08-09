#pragma once
#include "D3D12Device.h"
#include "D3D12DeviceChild.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Fence

class FD3D12Fence 
    : public FD3D12DeviceChild
{
public:
    FORCEINLINE FD3D12Fence(FD3D12Device* InDevice)
        : FD3D12DeviceChild(InDevice)
        , Fence(nullptr)
        , Event(0)
    { }

    FORCEINLINE ~FD3D12Fence()
    {
        if (Event)
        {
            CloseHandle(Event);
        }
    }

    bool Initialize(uint64 InitalValue)
    {
        HRESULT Result = GetDevice()->GetD3D12Device()->CreateFence(InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
        if (FAILED(Result))
        {
            D3D12_ERROR("FAILED to create Fence");
            return false;
        }

        Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (Event == 0)
        {
            D3D12_ERROR("FAILED to create Event for Fence");
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
            D3D12_ERROR("Failed to wait for fencevalue");
            return false;
        }
    }

    FORCEINLINE uint64 GetCompletedValue() const
    {
        return Fence->GetCompletedValue();
    }

    FORCEINLINE bool Signal(uint64 Value)
    {
        HRESULT Result = Fence->Signal(Value);
        return SUCCEEDED(Result);
    }

    FORCEINLINE void SetName(const FString& Name)
    {
        Fence->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), Name.GetCString());
    }

    FORCEINLINE ID3D12Fence* GetFence() const
    {
        return Fence.Get();
    }

private:
    TComPtr<ID3D12Fence> Fence;
    HANDLE               Event;
};