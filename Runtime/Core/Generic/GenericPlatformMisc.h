#pragma once
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR* Message) { }
    static FORCEINLINE bool IsDebuggerPresent() { return false; }
    static FORCEINLINE void MemoryBarrier() { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

