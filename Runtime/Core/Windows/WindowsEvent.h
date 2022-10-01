#pragma once
#include "Windows.h"

#include "Core/Generic/GenericEvent.h"

typedef TSharedRef<class FWindowsEvent> FWindowsEventRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsEvent

class CORE_API FWindowsEvent final
    : public FGenericEvent
{
public:
    FWindowsEvent()
        : Event(nullptr)
        , bManualReset(false)
    { }

    ~FWindowsEvent();

    bool Create(bool bInManualReset)
    {
        Event = ::CreateEventA(nullptr, bInManualReset, FALSE, nullptr);
        if (!Event)
        {
            return false;
        }

        bManualReset = bInManualReset;
        return true;
    }

    virtual void Trigger() override final;

    virtual void Wait(uint64 Milliseconds) override final;

    virtual void Reset() override final;

    virtual bool IsManualReset() const override final { return bManualReset; }

private:
    HANDLE Event;
    bool   bManualReset;
};