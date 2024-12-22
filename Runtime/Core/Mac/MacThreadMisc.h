#pragma once
#include "Core/Mac/MacThreadManager.h"
#include "Core/Generic/GenericThreadMisc.h"
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <Foundation/Foundation.h>

struct FMacThreadMisc final : public FGenericThreadMisc
{
    static FORCEINLINE uint32 GetNumProcessors()
    {
        const NSUInteger NumProcessors = [NSProcessInfo processInfo].processorCount;
        return static_cast<uint32>(NumProcessors);
    }

    static FORCEINLINE void* GetCurrentThreadHandle()
    {
        pthread_t CurrentThread = ::pthread_self();
        return reinterpret_cast<void*>(CurrentThread);
    }

    static FORCEINLINE void Sleep(FTimespan Time)
    {    
        float Microseconds = Time.AsMicroseconds();
        ::usleep(static_cast<useconds_t>(Microseconds));
    }

    static FORCEINLINE void Yield()
    {
        ::sched_yield();
    }

    static FORCEINLINE bool IsMainThread() 
    {
        return [NSThread isMainThread];
    }

    static FORCEINLINE void Pause() 
    {
        __builtin_ia32_pause();
    }
};
