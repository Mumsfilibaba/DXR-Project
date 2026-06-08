#include "Core/Windows/WindowsPlatformEvent.h"

FGenericPlatformEvent* FWindowsPlatformEvent::Create(bool bManualReset)
{
    FWindowsPlatformEvent* NewEvent = new FWindowsPlatformEvent();
    if (!NewEvent->Initialize(bManualReset))
    {
        delete NewEvent;
        return nullptr;
    }

    return NewEvent;
}

void FWindowsPlatformEvent::Recycle(FGenericPlatformEvent* InEvent)
{
    FWindowsPlatformEvent* WindowsEvent = static_cast<FWindowsPlatformEvent*>(InEvent);
    if (WindowsEvent)
    {
        delete WindowsEvent;
    }
}

FWindowsPlatformEvent::FWindowsPlatformEvent()
    : Event(nullptr)
    , bManualReset(false)
{
}

FWindowsPlatformEvent::~FWindowsPlatformEvent()
{
    if (Event != nullptr)
    {
        CloseHandle(Event);
    }
}

void FWindowsPlatformEvent::Trigger()
{
    CHECK(Event != nullptr);
    SetEvent(Event);
}

void FWindowsPlatformEvent::Wait(uint64 Milliseconds)
{
    CHECK(Event != nullptr);
    WaitForSingleObject(Event, static_cast<DWORD>(Milliseconds));
}

void FWindowsPlatformEvent::Reset()
{
    CHECK(Event != nullptr);
    ResetEvent(Event);
}
