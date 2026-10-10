#pragma once
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformTLS.h"

struct FWindowsPlatformTLS final : public IPlatformTLS
{
    static FORCEINLINE uint32 GetCurrentThreadID()
    {
        return ::GetCurrentThreadId();
    }

    static FORCEINLINE uint32 AllocTLSSlot(TLSDestructorFunc Destructor = nullptr)
    {
        return ::FlsAlloc(reinterpret_cast<PFLS_CALLBACK_FUNCTION>(Destructor));
    }

    static FORCEINLINE void SetTLSValue(uint32 SlotIndex, void* Value)
    {
        ::FlsSetValue(SlotIndex, Value);
    }

    static FORCEINLINE void* GetTLSValue(uint32 SlotIndex)
    {
        return ::FlsGetValue(SlotIndex);
    }

    static FORCEINLINE void FreeTLSSlot(uint32 SlotIndex)
    {
        ::FlsFree(SlotIndex);
    }
};
