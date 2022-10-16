#pragma once
#include "GenericThread.h"
#include "GenericEvent.h"

#include "Core/Time/Timespan.h"
#include "Core/Threading/ThreadInterface.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif


struct FGenericThreadMisc
{
    static FGenericThread* CreateThread(FThreadInterface* Runnable);
    
    static FGenericEvent* CreateEvent(bool bManualReset);

    static FORCEINLINE bool Initialize() { return true; }
    static FORCEINLINE void Release()    { }

    static FORCEINLINE uint32 GetNumProcessors() { return 1; }

    static FORCEINLINE void* GetThreadHandle() { return nullptr; }

    static FORCEINLINE void Sleep(FTimespan Time) { }

    static FORCEINLINE void Pause() { }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
