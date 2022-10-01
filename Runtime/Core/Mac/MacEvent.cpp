#include "MacEvent.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacEvent

FMacEvent::~FMacEvent()
{
    if (Event != nullptr)
    {
        CloseHandle(Event);
    }
}

void FMacEvent::Trigger()
{
    Check(Event != nullptr);
    SetEvent(Event);
}

void FMacEvent::Wait(uint64 Milliseconds)
{
    Check(Event != nullptr);
    WaitForSingleObject(Event, static_cast<DWORD>(Milliseconds));
}

void FMacEvent::Reset()
{
    Check(Event != nullptr);
    ResetEvent(Event);
}