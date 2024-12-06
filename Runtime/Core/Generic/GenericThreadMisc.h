#pragma once
#include "Core/Generic/GenericThread.h"
#include "Core/Generic/GenericEvent.h"
#include "Core/Time/Timespan.h"
#include "Core/Threading/Runnable.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericThreadMisc
{
    static FORCEINLINE void Release() { }
    static FORCEINLINE uint32 GetNumProcessors() { return 1; }
    static FORCEINLINE void* GetCurrentThreadHandle() { return nullptr; }
    static FORCEINLINE void Sleep(FTimespan Time) { }
	static FORCEINLINE void Yield() { }
    static FORCEINLINE void Pause() { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
