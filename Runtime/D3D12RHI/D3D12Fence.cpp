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
        D3D12_ERROR("[FD3D12Fence]: FAILED to create Fence");
        return false;
    }

    Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (Event == 0)
    {
        D3D12_ERROR("[FD3D12Fence]: FAILED to create Event for Fence");
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
        D3D12_ERROR("[FD3D12Fence]: Failed to wait for fencevalue");
        return false;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12FenceManager

FD3D12FenceManager::FD3D12FenceManager(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Fence(nullptr)
    , CurrentValue(0)
    , LastSignaledValue(0)
{ }

bool FD3D12FenceManager::Initialize()
{
    FD3D12FenceRef NewFence = dbg_new FD3D12Fence(GetDevice());
    if (!(NewFence && NewFence->Initialize(CurrentValue)))
    {
        return false;
    }

    Fence = NewFence;
    return true;
}

uint64 FD3D12FenceManager::SignalGPU(ED3D12CommandQueueType QueueType)
{
    Check(Fence != nullptr);

    ++CurrentValue;
    Check(LastSignaledValue != CurrentValue);

    ID3D12CommandQueue* CommandQueue = GetDevice()->GetD3D12CommandQueue(QueueType);
    Check(CommandQueue != nullptr);
    
    HRESULT hResult = CommandQueue->Signal(Fence->GetD3D12Fence(), CurrentValue);
    if (FAILED(hResult))
    {
        D3D12_ERROR("[FD3D12FenceManager]: Failed to signal Fence on the GPU");
    }

    LastSignaledValue = CurrentValue;
    return LastSignaledValue;
}

void FD3D12FenceManager::WaitGPU(ED3D12CommandQueueType QueueType)
{
    WaitGPU(QueueType, LastSignaledValue);
}

void FD3D12FenceManager::WaitGPU(ED3D12CommandQueueType QueueType, uint64 InFenceValue)
{
    Check(Fence != nullptr);
    Check(InFenceValue <= LastSignaledValue);

    ID3D12CommandQueue* CommandQueue = GetDevice()->GetD3D12CommandQueue(QueueType);
    Check(CommandQueue != nullptr);
    
    HRESULT hResult = CommandQueue->Wait(Fence->GetD3D12Fence(), InFenceValue);
    if (FAILED(hResult))
    {
        D3D12_ERROR("[FD3D12FenceManager]: Failed to wait for Fence on the GPU");
    }
}

void FD3D12FenceManager::WaitForFence()
{
    WaitForFence(LastSignaledValue);
}

void FD3D12FenceManager::WaitForFence(uint64 InFenceValue)
{
    Check(Fence != nullptr);
    Check(InFenceValue <= LastSignaledValue);

    if (InFenceValue >= GetCompletedValue())
    {
        Fence->WaitForValue(InFenceValue);
    }

    Check(InFenceValue <= LastCompletedValue);
}

uint64 FD3D12FenceManager::GetCompletedValue() const
{
    Check(Fence != nullptr);
    LastCompletedValue = Fence->GetD3D12Fence()->GetCompletedValue();
    return LastCompletedValue;
}
