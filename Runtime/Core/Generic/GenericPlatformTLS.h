#pragma once
#include "Core/Core.h"
#include "Core/Templates/NumericLimits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericPlatformTLS
{
    /**
     * @return - Returns the current thread ID
     */
    static FORCEINLINE uint32 GetCurrentThreadID() { return TNumericLimits<uint32>::Min(); }

    /**
     * @return - Allocates and returns a new TLS slot
     */
    static FORCEINLINE uint32 AllocTLSSlot() { return TNumericLimits<uint32>::Min(); }

    /**
     * @brief           - Set an allocated TLS value
     * @param SlotIndex - Index to set
     * @param Value     - Value to set
     */
    static FORCEINLINE void SetTLSValue(uint32 SlotIndex, void* Value) { }

    /**
     * @brief           - Retrieve the currently set TLS value
     * @param SlotIndex - Slot to retrieve the value for
     * @return          - Returns the current value
     */
    static FORCEINLINE void* GetTLSValue(uint32 SlotIndex) { return nullptr; }

    /**
     * @brief           - Free up the TLS slot
     * @param SlotIndex - Slot to free
     */
    static FORCEINLINE void FreeTLSSlot(uint32 SlotIndex) { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING