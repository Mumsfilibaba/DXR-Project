#include "Core/Windows/WindowsPlatformEvent.h"

IPlatformEvent* FWindowsPlatformEvent::CreateUnpooled(bool bManualReset)
{
    FWindowsPlatformEvent* NewEvent = new FWindowsPlatformEvent();
    if (!NewEvent->Initialize(bManualReset))
    {
        delete NewEvent;
        return nullptr;
    }

    return NewEvent;
}

void FWindowsPlatformEvent::DestroyUnpooled(IPlatformEvent* InEvent)
{
    delete static_cast<FWindowsPlatformEvent*>(InEvent);
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
