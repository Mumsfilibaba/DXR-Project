#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/PlatformTime.h"
#include "D3D11RHI/D3D11Fence.h"
#include "D3D11RHI/D3D11Device.h"

FD3D11FenceRHI::FD3D11FenceRHI(FD3D11Device* InDevice)
    : FRHIFence()
    , FD3D11DeviceChild(InDevice)
    , Fence(nullptr)
    , EventQuery(nullptr)
    , Event(0)
    , SignaledValue(0)
    , bHasPendingSignal(false)
{
}

FD3D11FenceRHI::~FD3D11FenceRHI()
{
    if (Event)
    {
        CloseHandle(Event);
    }
}

bool FD3D11FenceRHI::Initialize()
{
    ID3D11Device5* D3D11Device5 = GetDevice()->GetD3D11Device5();
    if (D3D11Device5 && GetDevice()->GetD3D11Context4())
    {
        HRESULT Result = D3D11Device5->CreateFence(0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
        if (SUCCEEDED(Result))
        {
            Event = ::CreateEventA(nullptr, FALSE, FALSE, nullptr);
            if (Event == 0)
            {
                D3D11_ERROR_CRITICAL("[FD3D11FenceRHI]: FAILED to create Event for Fence");
                return false;
            }

            return true;
        }

        Fence.Reset();
    }

    FD3D11QueryRef NewQuery = new FD3D11Query(GetDevice(), D3D11_QUERY_EVENT);
    if (!NewQuery->Initialize())
    {
        return false;
    }

    EventQuery = NewQuery;
    return true;
}

void FD3D11FenceRHI::Signal()
{
    if (Fence)
    {
        const uint64 NewValue = SignaledValue.Load() + 1;

        HRESULT Result = GetDevice()->GetD3D11Context4()->Signal(Fence.Get(), NewValue);
        if (FAILED(Result))
        {
            GetDevice()->CheckDeviceRemoved(Result, "Fence::Signal");
            D3D11_ERROR("[FD3D11FenceRHI]: Failed to signal Fence (0x%08X)", static_cast<uint32>(Result));
            return;
        }

        SignaledValue.Store(NewValue);
    }
    else
    {
        EventQuery->End(GetDevice()->GetD3D11Context());
    }

    bHasPendingSignal.Store(true);
}

bool FD3D11FenceRHI::IsSignaled() const
{
    if (!bHasPendingSignal.Load())
    {
        return false;
    }

    if (Fence)
    {
        return Fence->GetCompletedValue() >= SignaledValue.Load();
    }

    BOOL bSignaled = FALSE;
    return EventQuery->GetData(&bSignaled, sizeof(bSignaled), EQueryResultMode::Available) && bSignaled;
}

bool FD3D11FenceRHI::Wait(uint64 TimeoutNs) const
{
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

    if (Fence)
    {
        HRESULT Result = Fence->SetEventOnCompletion(SignaledValue.Load(), Event);
        if (FAILED(Result))
        {
            D3D11_ERROR("[FD3D11FenceRHI]: SetEventOnCompletion FAILED (0x%08X)", static_cast<uint32>(Result));
            return false;
        }

        return ::WaitForSingleObject(Event, TimeoutMs) == WAIT_OBJECT_0;
    }

    if (TimeoutMs == INFINITE)
    {
        BOOL bSignaled = FALSE;
        return EventQuery->GetData(&bSignaled, sizeof(bSignaled), EQueryResultMode::Wait) && bSignaled;
    }

    constexpr uint64 MillisecondsPerSecond = 1000ull;

    const uint64 Frequency = FPlatformTime::QueryPerformanceFrequency();
    const uint64 StartTime = FPlatformTime::QueryPerformanceCounter();
    while (!IsSignaled())
    {
        const uint64 ElapsedMs = ((FPlatformTime::QueryPerformanceCounter() - StartTime) * MillisecondsPerSecond) / Frequency;
        if (ElapsedMs >= TimeoutMs)
        {
            return false;
        }

        FPlatformThreadMisc::Yield();
    }

    return true;
}

void* FD3D11FenceRHI::GetRHINativeFence() const
{
    if (Fence)
    {
        return reinterpret_cast<void*>(Fence.Get());
    }

    return EventQuery ? reinterpret_cast<void*>(EventQuery->GetD3D11Query()) : nullptr;
}

void FD3D11FenceRHI::SetDebugName(const String& InName)
{
    if (Fence)
    {
        D3D11SetDebugName(Fence.Get(), InName);
    }
    else if (EventQuery)
    {
        EventQuery->SetDebugName(InName);
    }
}

void FD3D11FenceRHI::GetDebugName(String& OutDebugName) const
{
    if (Fence)
    {
        D3D11GetDebugName(Fence.Get(), OutDebugName);
    }
    else if (EventQuery)
    {
        EventQuery->GetDebugName(OutDebugName);
    }
}
