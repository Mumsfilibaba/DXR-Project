#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12Device.h"

FD3D12Fence::FD3D12Fence(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Fence(nullptr)
    , Event(0)
    , LastCompletedValue(0)
    , CurrentValue(0)
    , LastSignaledValue(0)
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
    CurrentValue     = InitialValue;
    LastSignaledValue = InitialValue;

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

uint64 FD3D12Fence::Signal(ID3D12CommandQueue* Queue)
{
    CHECK(Queue != nullptr);

    ++CurrentValue;
    CHECK(LastSignaledValue != CurrentValue);

    HRESULT hResult = Queue->Signal(Fence.Get(), CurrentValue);
    if (FAILED(hResult))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Fence]: Failed to signal Fence on the GPU");
    }

    LastSignaledValue = CurrentValue;
    return LastSignaledValue;
}

void FD3D12Fence::WaitGPU(ID3D12CommandQueue* Queue, uint64 Value)
{
    CHECK(Queue != nullptr);
    CHECK(Fence != nullptr);
    CHECK(Value <= LastSignaledValue);

    HRESULT hResult = Queue->Wait(Fence.Get(), Value);
    if (FAILED(hResult))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Fence]: Failed to wait for Fence on the GPU");
    }
}

bool FD3D12Fence::WaitForValue(uint64 Value, uint32 TimeoutMs)
{
    uint64 CompletedValue = GetCompletedValue();
    if (Value <= CompletedValue)
    {
        return true;
    }

    HRESULT Result = Fence->SetEventOnCompletion(Value, Event);
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Fence]: SetEventOnCompletion Failed");
        return false;
    }

    DWORD WaitResult = ::WaitForSingleObject(Event, TimeoutMs);
    if (WaitResult == WAIT_OBJECT_0)
    {
        GetCompletedValue();
        return true;
    }

    return false;
}

uint64 FD3D12Fence::GetCompletedValue() const
{
    CHECK(Fence != nullptr);
    LastCompletedValue = Fence->GetCompletedValue();
    return LastCompletedValue;
}

void FD3D12Fence::SetDebugName(const FString& Name)
{
    CHECK(Fence != nullptr);
    Fence->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Length(), *Name);
}

FD3D12FenceRHI::FD3D12FenceRHI(FD3D12Device* InDevice)
    : FRHIFence()
    , FD3D12DeviceChild(InDevice)
    , Fence(nullptr)
    , bHasPendingSignal(false)
    , DebugName()
{
}

bool FD3D12FenceRHI::Initialize()
{
    FD3D12FenceRef NewFence = new FD3D12Fence(GetDevice());
    if (!(NewFence && NewFence->Initialize(0)))
    {
        return false;
    }

    Fence = NewFence;
    return true;
}

void FD3D12FenceRHI::Signal(ID3D12CommandQueue* Queue)
{
    CHECK(Fence != nullptr);
    bHasPendingSignal.Store(true);
    Fence->Signal(Queue);
}

bool FD3D12FenceRHI::IsSignaled() const
{
    CHECK(Fence != nullptr);

    if (!bHasPendingSignal.Load())
    {
        return false;
    }

    return Fence->GetLastSignaledValue() <= Fence->GetCompletedValue();
}

bool FD3D12FenceRHI::Wait(uint64 TimeoutNs) const
{
    CHECK(Fence != nullptr);

    if (!bHasPendingSignal.Load())
    {
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

    return Fence->WaitForValue(Fence->GetLastSignaledValue(), TimeoutMs);
}

void FD3D12FenceRHI::SetDebugName(const FString& InName)
{
    DebugName = InName;
    if (Fence)
    {
        Fence->SetDebugName(InName);
    }
}

FString FD3D12FenceRHI::GetDebugName() const
{
    return DebugName;
}
