#pragma once
#include "MacRunLoop.h"

#include "Core/Threading/Generic/GenericThreadMisc.h"

#include <unistd.h>
#include <pthread.h>
#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacThreadMisc

struct FMacThreadMisc 
    : public FGenericThreadMisc
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericThreadMisc Interface

    static FGenericThread* CreateThread(const TFunction<void()>& InFunction);
    static FGenericThread* CreateNamedThread(const TFunction<void()>& InFunction, const FString& InString);

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

    static FORCEINLINE void Sleep(FTimestamp Time)
    {	
        float MicroSeconds = Time.AsMicroSeconds();
        usleep(static_cast<useconds_t>(MicroSeconds));
    }

    static FORCEINLINE bool IsMainThread() 
    {
		return [NSThread isMainThread];
    }
};
