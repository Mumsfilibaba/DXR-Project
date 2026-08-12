#pragma once
#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/PlatformInterface/IPlatformEvent.h"
#include "Core/Time/Timespan.h"
#include "Core/Threading/Runnable.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformThreadMisc
{
    /** @brief Release any threading resources the platform acquired, called when the ThreadManager shuts down */
    static FORCEINLINE void Release()
    {
    }

    /** @return Returns the number of logical processors available to the process */
    static FORCEINLINE uint32 GetNumProcessors()
    {
        return 1;
    }

    /** @return Returns the native handle of the calling thread */
    static FORCEINLINE void* GetCurrentThreadHandle()
    {
        return nullptr;
    }

    /** @brief Block the calling thread for at least the specified duration */
    static FORCEINLINE void Sleep(FTimespan Time)
    {
    }

    /** @brief Give up the remainder of the calling thread's time-slice to another runnable thread */
    static FORCEINLINE void Yield()
    {
    }

    /** @brief Hint to the processor that the calling thread is in a spin-wait, used to back off inside a spinlock */
    static FORCEINLINE void Pause()
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
