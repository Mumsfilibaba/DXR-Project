#pragma once
#include "Core/Threading/Atomic.h"
#include "Core/Platform/PlatformThreadMisc.h"

class FSpinLock
{
    enum
    {
        STATE_UNLOCKED = 0,
        STATE_LOCKED   = 1,
    };

public:
    FSpinLock(const FSpinLock&) = delete;
    FSpinLock& operator=(const FSpinLock&) = delete;

    ~FSpinLock() = default;

    /** @brief - Default constructor */
    FORCEINLINE FSpinLock() noexcept
        : State(STATE_UNLOCKED)
    {
    }

    /** @brief - Lock SpinLock for other threads */
    FORCEINLINE void Lock() noexcept
    {
        // Try locking until success
        for (;;)
        {
            // When the previous value is unlocked => success
            if (State.Exchange(STATE_LOCKED) == STATE_UNLOCKED)
            {
                break;
            }

            while (State.RelaxedLoad() == STATE_LOCKED)
            {
                FPlatformThreadMisc::Pause();
            }
        }
    }

    /** @return - Tries to lock CriticalSection for other threads and, returns true if the lock is successful */
    FORCEINLINE bool TryLock() noexcept
    {
        // The first relaxed load is in order to prevent unnecessary cache misses when trying to lock in a loop: See Lock
        return (State.RelaxedLoad() == STATE_UNLOCKED) && (State.Exchange(STATE_LOCKED) == STATE_UNLOCKED);
    }

    /** @brief - Unlock CriticalSection for other threads */
    FORCEINLINE void Unlock() noexcept
    {
        State.Store(STATE_UNLOCKED);
    }

private:
    FAtomicInt32 State;
};
