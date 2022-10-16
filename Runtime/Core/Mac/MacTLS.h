#pragma once
#include "Mac.h"

#include "Core/Generic/GenericTLS.h"


struct FMacTLS 
    : public FGenericTLS
{
    static FORCEINLINE uint32 GetCurrentThreadID()
    {
        return static_cast<uint32>(pthread_mach_thread_np(pthread_self()));
    }

    static FORCEINLINE uint32 AllocTLSSlot()
    {
        pthread_key_t Slot = 0;
        if (pthread_key_create(&Slot) != 0)
        {
            // Same as FWindowsTLS
            Slot = TNumericLimits<uint32>::Max();
        }

        return static_cast<uint32>(Slot);
    }

    static FORCEINLINE void SetTLSValue(uint32 SlotIndex, void* Value) 
    {
        pthread_setspecific(static_cast<pthread_key_t>(SlotIndex), Value);
    }

    static FORCEINLINE void* GetTLSValue(uint32 SlotIndex) 
    { 
        return pthread_getspecific(static_cast<pthread_key_t>(SlotIndex));
    }

    static FORCEINLINE void FreeTLSSlot(uint32 SlotIndex) 
    {
        pthread_key_delete(static_cast<pthread_key_t>(SlotIndex));
    }
};