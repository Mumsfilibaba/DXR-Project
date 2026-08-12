#pragma once
#include "Core/Time/Timespan.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformEvent
{
public:

    /** @brief Take an event from the pool, creating one when the pool holds none of this reset mode */
    static IPlatformEvent* Create(bool bManualReset);

    /** @brief Return an event to the pool */
    static void Recycle(IPlatformEvent* InEvent);

    /** @brief Raw OS alloc */
    static IPlatformEvent* CreateUnpooled(bool bManualReset);

    /** @brief Raw OS free */
    static void DestroyUnpooled(IPlatformEvent* InEvent);

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
