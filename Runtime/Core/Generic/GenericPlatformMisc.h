#pragma once
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class EAssertDialogResult
{
    Abort  = 0,
    Debug  = 1,
    Ignore = 2,
};

struct FGenericPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR* Message) { }
    static FORCEINLINE void WriteToStdOutput(const CHAR* Text, uint32 Length) { }
    static FORCEINLINE bool IsDebuggerPresent() { return false; }
    static FORCEINLINE void MemoryBarrier() { }

    static FORCEINLINE EAssertDialogResult ShowAssertDialog(const CHAR* Title, const CHAR* Message)
    {
        return EAssertDialogResult::Abort;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

