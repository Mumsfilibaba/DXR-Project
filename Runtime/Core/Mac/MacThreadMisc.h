#pragma once
#include "MacRunLoop.h"

#include "Core/Generic/GenericThreadMisc.h"

#include <unistd.h>
#include <pthread.h>
#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacThreadMisc

struct FMacThreadMisc 
    : public FGenericThreadMisc
{
    static FGenericThread* CreateThread(const FThreadFunction& InFunction);
    static FGenericThread* CreateNamedThread(const FThreadFunction& InFunction, const FString& InString);

    static FORCEINLINE bool Initialize() 
    { 
        Check(IsMainThread());
		    return RegisterMainRunLoop();
    }

    static FORCEINLINE void Release() 
    {
		UnregisterMainRunLoop();
    }

    static uint32 GetNumProcessors()
    {
        NSUInteger NumProcessors = [NSProcessInfo processInfo].processorCount;
        return static_cast<uint32>(NumProcessors);
    }

    static FORCEINLINE void* GetThreadHandle()
    {
        pthread_t CurrentThread = pthread_self();
        return reinterpret_cast<void*>(CurrentThread);
    }

    static FORCEINLINE void Sleep(FTimespan Time)
    {	
        float Microseconds = Time.AsMicroseconds();
        usleep(static_cast<useconds_t>(Microseconds));
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