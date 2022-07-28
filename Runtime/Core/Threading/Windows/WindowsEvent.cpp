#include "WindowsEvent.h"


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsEvent

FWindowsEvent::~FWindowsEvent()
{
    if (Event != nullptr)
    {
        CloseHandle(Event);
    }
}

void FWindowsEvent::Trigger()
{
    Check(Event != nullptr);
    SetEvent(Event);
}

void FWindowsEvent::Wait(uint64 Milliseconds)
{
    Check(Event != nullptr);
    WaitForSingleObject(Event, static_cast<DWORD>(Milliseconds));
}

void FWindowsEvent::Reset()
{
    Check(Event != nullptr);
    ResetEvent(Event);
}