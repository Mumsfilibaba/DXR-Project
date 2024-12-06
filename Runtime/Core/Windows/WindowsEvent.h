#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Generic/GenericEvent.h"

class CORE_API FWindowsEvent final : public FGenericEvent
{
public:
    static FGenericEvent* Create(bool bManualReset);
    static void Recycle(FGenericEvent* InEvent);

    virtual void Trigger() override final;
    virtual void Wait(uint64 Milliseconds) override final;
    virtual void Reset() override final;

    virtual bool IsManualReset() const override final 
    { 
        return bManualReset;
    }

private:
    FWindowsEvent();
    ~FWindowsEvent();

    bool Initialize(bool bInManualReset)
    {
        Event = ::CreateEventA(nullptr, bInManualReset, FALSE, nullptr);
        if (!Event)
        {
            return false;
        }

        bManualReset = bInManualReset;
        return true;
    }

    HANDLE Event;
    bool   bManualReset;
};