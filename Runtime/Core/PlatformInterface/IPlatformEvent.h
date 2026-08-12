#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformEvent
{
public:

    /** @brief Creates a new event. Each platform hides this with its own */
    static IPlatformEvent* Create(bool bManualReset)
    {
        return nullptr;
    }

    /** @brief Return the event to the system for reuse if possible. Each platform hides this with its own */
    static void Recycle(IPlatformEvent* InEvent)
    {
    }

public:

    /** @brief Trigger the event */
    virtual void Trigger() = 0;

    /** @brief Wait for the event to be triggered */
    virtual void Wait(uint64 Milliseconds) = 0;

    /** @brief Wait for the event to be triggered */
    virtual void Wait(FTimespan Timeout) = 0;

    /** @brief Reset the event */
    virtual void Reset() = 0;

    /** @brief Check if the event needs a manual reset */
    virtual bool IsManualReset() const = 0;

protected:
    virtual ~IPlatformEvent() = default;
    IPlatformEvent() = default;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
