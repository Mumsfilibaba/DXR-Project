#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12Device.h"

FD3D12Fence::FD3D12Fence(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Fence(nullptr)
    , Event(0)
{
}

FD3D12Fence::~FD3D12Fence()
{
    if (Event)
    {
        CloseHandle(Event);
    }
}

bool FD3D12Fence::Initialize(uint64 InitialValue)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateFence(InitialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Fence]: FAILED to create Fence");
        return false;
    }

    Event = ::CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (Event == 0)
    {
        D3D12_ERROR_CRITICAL("[FD3D12Fence]: FAILED to create Event for Fence");
        return false;
    }

    return true;
}

bool FD3D12Fence::WaitForValue(uint64 Value, uint32 TimeoutMs)
{
    HRESULT Result = Fence->SetEventOnCompletion(Value, Event);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Fence]: SetEventOnCompletion Failed");
        return false;
    }

    // NOTE: Check if the object was signaled, could also have timed out
    DWORD WaitResult = ::WaitForSingleObject(Event, TimeoutMs);
    return WaitResult == WAIT_OBJECT_0;
}

FD3D12FenceManager::FD3D12FenceManager(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Fence(nullptr)
    , LastCompletedValue(0)
    , CurrentValue(0)
    , LastSignaledValue(0)
{
}

bool FD3D12FenceManager::Initialize()
{
    FD3D12FenceRef NewFence = new FD3D12Fence(GetDevice());
    if (!(NewFence && NewFence->Initialize(CurrentValue)))
    {
        return false;
    }

    Fence = NewFence;
    return true;
}

void FD3D12FenceManager::Release()
{
    Fence.Reset();
}

uint64 FD3D12FenceManager::SignalGPU(ED3D12CommandQueueType QueueType)
{
    ++CurrentValue;
    CHECK(LastSignaledValue != CurrentValue);

    ID3D12CommandQueue* CommandQueue = GetDevice()->GetD3D12CommandQueue(QueueType);
    CHECK(CommandQueue != nullptr);
    
    HRESULT hResult = CommandQueue->Signal(Fence->GetD3D12Fence(), CurrentValue);
    if (FAILED(hResult))
    {
        D3D12_ERROR_CRITICAL("[FD3D12FenceManager]: Failed to signal Fence on the GPU");
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
    CHECK(Fence != nullptr);
    CHECK(InFenceValue <= LastSignaledValue);

    ID3D12CommandQueue* CommandQueue = GetDevice()->GetD3D12CommandQueue(QueueType);
    CHECK(CommandQueue != nullptr);
    
    HRESULT hResult = CommandQueue->Wait(Fence->GetD3D12Fence(), InFenceValue);
    if (FAILED(hResult))
    {
        D3D12_ERROR_CRITICAL("[FD3D12FenceManager]: Failed to wait for Fence on the GPU");
    }
}

void FD3D12FenceManager::WaitForFence()
{
    WaitForFence(LastSignaledValue);
}

void FD3D12FenceManager::WaitForFence(uint64 InFenceValue)
{
    CHECK(Fence != nullptr);
    CHECK(InFenceValue <= LastSignaledValue);

    uint64 CompletedFenceValue = GetCompletedValue();
    if (InFenceValue > CompletedFenceValue)
    {
        Fence->WaitForValue(InFenceValue);
        CompletedFenceValue = GetCompletedValue();
    }

    CHECK(InFenceValue <= CompletedFenceValue);
}

uint64 FD3D12FenceManager::GetCompletedValue() const
{
    CHECK(Fence != nullptr);
    LastCompletedValue = Fence->GetD3D12Fence()->GetCompletedValue();
    return LastCompletedValue;
}

FD3D12GpuFence::FD3D12GpuFence(FD3D12Device* InDevice)
    : FRHIGpuFence()
    , FD3D12DeviceChild(InDevice)
    , Fence(nullptr)
    , CurrentValue(0)
    , TargetValue(0)
    , bHasPendingSignal(false)
    , DebugName()
{
}

bool FD3D12GpuFence::Initialize()
{
    FD3D12FenceRef NewFence = new FD3D12Fence(GetDevice());
    if (!(NewFence && NewFence->Initialize(0)))
    {
        return false;
    }

    Fence = NewFence;
    return true;
}

void FD3D12GpuFence::Signal(ED3D12CommandQueueType QueueType)
{
    CHECK(Fence != nullptr);

    ++CurrentValue;
    TargetValue = CurrentValue;
    bHasPendingSignal.Store(true);

    ID3D12CommandQueue* CommandQueue = GetDevice()->GetD3D12CommandQueue(QueueType);
    CHECK(CommandQueue != nullptr);

    HRESULT Result = CommandQueue->Signal(Fence->GetD3D12Fence(), TargetValue);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12GpuFence]: Failed to signal Fence on the GPU");
    }
}

bool FD3D12GpuFence::IsSignaled() const
{
    CHECK(Fence != nullptr);

    if (!bHasPendingSignal.Load())
    {
        // A fence that has never been signaled should not be treated as completed.
        return false;
    }

    return TargetValue <= Fence->GetCompletedValue();
}

bool FD3D12GpuFence::Wait(uint64 TimeoutNs) const
{
    CHECK(Fence != nullptr);

    if (!bHasPendingSignal.Load())
    {
        // Nothing has been enqueued to signal this fence yet.
        return false;
    }

    if (IsSignaled())
    {
        return true;
    }

    uint32 TimeoutMs = INFINITE;
    if (TimeoutNs != UINT64_MAX)
    {
        constexpr uint64 NanosecondsPerMillisecond = 1000ull * 1000ull;
        const uint64 TimeoutMs64 = (TimeoutNs + (NanosecondsPerMillisecond - 1)) / NanosecondsPerMillisecond;
        TimeoutMs = TimeoutMs64 > UINT32_MAX ? UINT32_MAX : static_cast<uint32>(TimeoutMs64);
    }

    return Fence->WaitForValue(TargetValue, TimeoutMs);
}

void FD3D12GpuFence::SetDebugName(const FString& InName)
{
    DebugName = InName;
    if (Fence)
    {
        Fence->SetDebugName(InName);
    }
}

FString FD3D12GpuFence::GetDebugName() const
{
    return DebugName;
}
