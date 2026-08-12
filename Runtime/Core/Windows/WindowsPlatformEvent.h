#pragma once
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformEvent.h"

class CORE_API FWindowsPlatformEvent final : public IPlatformEvent
{
public:
    static IPlatformEvent* CreateUnpooled(bool bManualReset);
    static void DestroyUnpooled(IPlatformEvent* InEvent);

public:
    virtual void Trigger() override final;
    virtual void Wait(uint64 Milliseconds) override final;
    virtual void Reset() override final;

    virtual void Wait(FTimespan Timeout) override final
    {
        Wait(static_cast<uint64>(Timeout.AsMilliseconds()));
    }

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
