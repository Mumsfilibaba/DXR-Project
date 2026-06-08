#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Generic/GenericPlatformEvent.h"

class CORE_API FWindowsPlatformEvent final : public FGenericPlatformEvent
{
public:
    static FGenericPlatformEvent* Create(bool bManualReset);
    static void Recycle(FGenericPlatformEvent* InEvent);

    virtual void Trigger() override final;
    virtual void Wait(uint64 Milliseconds) override final;
    virtual void Reset() override final;

    virtual bool IsManualReset() const override final 
    { 
        return bManualReset;
    }

private:
    FWindowsPlatformEvent();
    ~FWindowsPlatformEvent();

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
