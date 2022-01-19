#pragma once
#include "Core/Core.h"
#include "AtomicInt.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A simple lock that does not use OS functions to lock a thread. Good for short locking periods in high contingency areas

class CSpinLock
{
    enum
    {
        State_Unlocked = 0,
        State_Locked = 1,
    };

public:

    CSpinLock(const CSpinLock&) = delete;
    CSpinLock& operator=(const CSpinLock&) = delete;

    FORCEINLINE CSpinLock() noexcept
        : State(State_Unlocked)
    {
    }

    ~CSpinLock() = default;

    FORCEINLINE void Lock() noexcept
    {
        // Try locking until success
        for (;; )
        {
            // When the previous value is unlocked => success
            if (State.Exchange(State_Locked) == State_Unlocked)
            {
                break;
            }

            while (State.RelaxedLoad() == State_Locked)
            {
                PauseInstruction();
            }
        }
    }

    FORCEINLINE bool TryLock() noexcept
    {
        // The first relaxed load is in order to prevent unnecessary cache misses when trying to lock in a loop: See Lock
        return (State.RelaxedLoad() == State_Unlocked) && (State.Exchange(State_Locked) == State_Unlocked);
    }

    FORCEINLINE void Unlock() noexcept
    {
        State.Store(State_Unlocked);
    }

private:
    AtomicInt32 State;
};
