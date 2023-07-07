#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct CORE_API FGenericEvent : public FRefCounted
{
    virtual ~FGenericEvent() = default;

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
};

ENABLE_UNREFERENCED_VARIABLE_WARNING