#include "WindowsEvent.h"

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