#include "D3D12Fence.h"
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Fence

FD3D12Fence::FD3D12Fence(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Fence(nullptr)
    , Event(0)
{ }

FD3D12Fence::~FD3D12Fence()
{
    if (Event)
    {
        CloseHandle(Event);
    }
}

bool FD3D12Fence::Initialize(uint64 InitalValue)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateFence(
        InitalValue, 
        D3D12_FENCE_FLAG_NONE, 
        IID_PPV_ARGS(&Fence));

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

bool FD3D12Fence::WaitForValue(uint64 Value)
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