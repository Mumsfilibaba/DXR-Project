#pragma once
#include "Windows.h"

#include "Core/Generic/GenericPlatformTLS.h"

struct FWindowsPlatformTLS
    : public FGenericPlatformTLS
{
    static FORCEINLINE uint32 GetCurrentThreadID()
    {
        return ::GetCurrentThreadId();
    }

    static FORCEINLINE uint32 AllocTLSSlot()
    {
        return ::TlsAlloc();
    }

    static FORCEINLINE void SetTLSValue(uint32 SlotIndex, void* Value)
    {
        ::TlsSetValue(SlotIndex, Value);
    }

    static FORCEINLINE void* GetTLSValue(uint32 SlotIndex)
    {
        return ::TlsGetValue(SlotIndex);
    }

    static FORCEINLINE void FreeTLSSlot(uint32 SlotIndex)
    {
        ::TlsFree(SlotIndex);
    }
};