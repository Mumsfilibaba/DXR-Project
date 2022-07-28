#pragma once
#include "Core/Core.h"
#include "AtomicInt.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FSpinLock

class FSpinLock
{
    enum
    {
        State_Unlocked = 0,
        State_Locked   = 1,
    };

public:
    FSpinLock(const FSpinLock&) = delete;
    FSpinLock& operator=(const FSpinLock&) = delete;

    ~FSpinLock() = default;

    /** @brief: Default constructor */
    FORCEINLINE FSpinLock() noexcept
        : State(State_Unlocked)
    { }

    /** @brief: Lock SpinLock for other threads */
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

    /** @return: Tries to lock CriticalSection for other threads and, returns true if the lock is successful */
    FORCEINLINE bool TryLock() noexcept
    {
        // The first relaxed load is in order to prevent unnecessary cache misses when trying to lock in a loop: See Lock
        return (State.RelaxedLoad() == State_Unlocked) && (State.Exchange(State_Locked) == State_Unlocked);
    }

    /** @brief: Unlock CriticalSection for other threads */
    FORCEINLINE void Unlock() noexcept
    {
        State.Store(State_Unlocked);
    }

private:
    FAtomicInt32 State;
};
