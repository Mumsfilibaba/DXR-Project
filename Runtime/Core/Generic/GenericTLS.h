#pragma once
#include "Core/Core.h"
#include "Core/Templates/NumericLimits.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericTLS

struct FGenericTLS
{
	/**
	 * @return Returns the current thread ID
	 */
	static FORCEINLINE uint32 GetCurrentThreadID()
	{
		return TNumericLimits<uint32>::Min();
	}

	/**
	 * @return Allocates and returns a new TLS slot
	 */
	static FORCEINLINE uint32 AllocTLSSlot()
	{
		return TNumericLimits<uint32>::Min();
	}

	/**
	 * @brief Set an allocated TLS value
	 * 
	 * @param SlotIndex: Index to set
	 * @param Value: Value to set
	 */
	static FORCEINLINE void SetTLSValue(uint32 SlotIndex, void* Value) { }

	/**
	 * @brief Retrieve the currently set TLS value
	 * 
	 * @param SlotIndex: Slot to retrieve the value for
	 * @return Returns the current value
	 */
	static FORCEINLINE void* GetTLSValue(uint32 SlotIndex) { return nullptr; }

	/**
	 * @brief Free up the TLS slot
	 * 
	 * @param SlotIndex: Slot to free
	 */
	static FORCEINLINE void FreeTLSSlot(uint32 SlotIndex) { }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif