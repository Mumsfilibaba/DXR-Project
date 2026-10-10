#pragma once
#include "Core/Core.h"
#include "Core/Templates/NumericLimits.h"

#ifndef CORE_INVALID_TLS_INDEX
    #define CORE_INVALID_TLS_INDEX uint32(0xffffffff)
#endif

#ifndef CORE_INVALID_THREAD_ID
    #define CORE_INVALID_THREAD_ID uint32(0)
#endif

DISABLE_UNREFERENCED_VARIABLE_WARNING

/** @brief Receives a thread's value in a TLS slot when that thread exits */
using TLSDestructorFunc = void (*)(void* Value);

struct IPlatformTLS
{
    /** @return Returns the current thread ID */
    static FORCEINLINE uint32 GetCurrentThreadID()
    {
        return TNumericLimits<uint32>::Min();
    }

    /**
     * @brief Allocates a new TLS slot
     *
     * @param Destructor Called with a thread's value when that thread exits, unless the value is nullptr. It is not
     * called for the main thread when the process exits.
     * @return Returns the new slot, or CORE_INVALID_TLS_INDEX when no slot is available
     */
    static FORCEINLINE uint32 AllocTLSSlot(TLSDestructorFunc Destructor = nullptr)
    {
        return TNumericLimits<uint32>::Max();
    }

    /**
     * @brief Set an allocated TLS value
     * @param SlotIndex Index to set
     * @param Value Value to set
     */
    static FORCEINLINE void SetTLSValue(uint32 SlotIndex, void* Value)
    {
    }

    /**
     * @brief Retrieve the currently set TLS value
     * @param SlotIndex Slot to retrieve the value for
     * @return Returns the current value
     */
    static FORCEINLINE void* GetTLSValue(uint32 SlotIndex)
    {
        return nullptr;
    }

    /**
     * @brief Free up the TLS slot
     * @param SlotIndex Slot to free
     */
    static FORCEINLINE void FreeTLSSlot(uint32 SlotIndex)
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING