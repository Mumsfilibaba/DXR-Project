#pragma once

#if PLATFORM_MACOS
#include "Core/CoreModule.h"

#include <CoreFoundation/CoreFoundation.h>

#ifdef __OBJC__
#include <Foundation/Foundation.h>
#else
class NSString;
#endif

class CMacRunLoopSource;

class CORE_API CMacMainThread
{
public:

    static void Init();
    static void Release();
    
    static FORCEINLINE IsInitialized()
    {
        return (MainThread != nullptr);
    }

    static void Tick();
    
    static void MakeCall( dispatch_block_t Block, bool WaitUntilFinished );
    
private:
    static CMacRunLoopSource* MainThread;
};

inline void MakeMainThreadCall( dispatch_block_t Block, bool WaitUntilFinished )
{
    CMacMainThread::MakeCall( Block, WaitUntilFinished );
}

#endif