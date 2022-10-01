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
	static FORCEINLINE uint32 GetCurrentThreadID()
	{
		return TNumericLimits<uint32>::Min();
	}

	static FORCEINLINE uint32 AllocTLSSlot()
	{
		return TNumericLimits<uint32>::Min();
	}

	static FORCEINLINE void SetTLSValue(uint32 SlotIndex, void* Value) { }

	static FORCEINLINE void* GetTLSValue(uint32 SlotIndex) { return nullptr; }

	static FORCEINLINE void FreeTLSSlot(uint32 SlotIndex) { }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif