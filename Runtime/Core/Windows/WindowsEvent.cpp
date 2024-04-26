#include "WindowsEvent.h"

FGenericEvent* FWindowsEvent::Create(bool bManualReset)
{
    FWindowsEvent* NewEvent = new FWindowsEvent();
    if (!NewEvent->Initialize(bManualReset))
    {
        delete NewEvent;
        return nullptr;
    }

    return NewEvent;
}

void FWindowsEvent::Recycle(FGenericEvent* InEvent)
{
    FWindowsEvent* WindowsEvent = static_cast<FWindowsEvent*>(InEvent);
    if (WindowsEvent)
    {
        delete WindowsEvent;
    }
}

FWindowsEvent::FWindowsEvent()
    : Event(nullptr)
    , bManualReset(false)
{
}

FWindowsEvent::~FWindowsEvent()
{
    if (Event != nullptr)
    {
        CloseHandle(Event);
    }
}

void FWindowsEvent::Trigger()
{
    CHECK(Event != nullptr);
    SetEvent(Event);
}

void FWindowsEvent::Wait(uint64 Milliseconds)
{
    CHECK(Event != nullptr);
    WaitForSingleObject(Event, static_cast<DWORD>(Milliseconds));
}

void FWindowsEvent::Reset()
{
    CHECK(Event != nullptr);
    ResetEvent(Event);
}