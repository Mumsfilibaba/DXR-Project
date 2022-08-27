#pragma once
#include "Core/Time/Timespan.h"
#include "Core/Containers/SharedRef.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

typedef TSharedRef<struct FGenericEvent> FGenericEventRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericEvent

struct CORE_API FGenericEvent
    : public FRefCounted
{
    /** @brief: Trigger the event */
    virtual void Trigger() { }

    /** @brief: Wait for the event to be triggered */
    virtual void Wait(uint64 Milliseconds) { }

    /** @brief: Wait for the event to be triggered */
    virtual void Wait(FTimespan Timeout) { Wait(static_cast<uint64>(Timeout.AsMilliseconds())); }

    /** @brief: Reset the event */
    virtual void Reset() { }

    /** @brief: Check if the event needs a manual reset */
    virtual bool IsManualReset() const { return false; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif