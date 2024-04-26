#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class CORE_API FGenericEvent : public FRefCounted
{
public:
    // Creates a new event
    static FGenericEvent* Create(bool bManualReset);

    // Return the event to the system for reuse if possible
    static void Recycle(FGenericEvent* InEvent);

    /** @brief - Trigger the event */
    virtual void Trigger() { }

    /** @brief - Wait for the event to be triggered */
    virtual void Wait(uint64 Milliseconds) { }

    /** @brief - Wait for the event to be triggered */
    virtual void Wait(FTimespan Timeout) { Wait(static_cast<uint64>(Timeout.AsMilliseconds())); }

    /** @brief - Reset the event */
    virtual void Reset() { }

    /** @brief - Check if the event needs a manual reset */
    virtual bool IsManualReset() const { return false; }

protected:

    // Protected, creation and destruction should be handled by the static functions
    FGenericEvent() = default;
    virtual ~FGenericEvent() = default;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING