#pragma once
#include "GenericThread.h"
#include "GenericEvent.h"

#include "Core/Time/Timespan.h"
#include "Core/Threading/ThreadInterface.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING


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

ENABLE_UNREFERENCED_VARIABLE_WARNING
