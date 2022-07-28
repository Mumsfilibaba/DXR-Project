#pragma once
#include "Core/Threading/Generic/GenericEvent.h"

typedef TSharedRef<class FWindowsEvent> FWindowsEventRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsEvent

class CORE_API FWindowsEvent final
    : public FGenericEvent
{
    friend struct FWindowsThreadMisc;

private:
    FWindowsEvent()
        : Event(nullptr)
        , bManualReset(false)
    { }

    ~FWindowsEvent();

public:
    bool Create(bool bInManualReset)
    {
        Event = CreateEvent(nullptr, bInManualReset, FALSE, nullptr);
        if (Event != nullptr)
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