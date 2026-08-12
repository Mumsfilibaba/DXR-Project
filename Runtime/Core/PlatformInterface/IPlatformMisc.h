#pragma once
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class EAssertDialogResult
{
    Abort  = 0,
    Debug  = 1,
    Ignore = 2,
};

struct IPlatformMisc
{
    /** @brief Write a message to the platform's debugger output, terminated by a newline */
    static FORCEINLINE void OutputDebugString(const CHAR* Message)
    {
    }

    /** @brief Write Length characters of Text to standard output */
    static FORCEINLINE void WriteToStdOutput(const CHAR* Text, uint32 Length)
    {
    }

    /** @return Returns true if a debugger is currently attached to the process */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return false;
    }

    /** @brief Issue a memory barrier, stopping memory operations from being reordered across this point */
    static FORCEINLINE void MemoryBarrier()
    {
    }

    /** @return Returns how the user chose to respond to a failed assert */
    static FORCEINLINE EAssertDialogResult ShowAssertDialog(const CHAR* Title, const CHAR* Message)
    {
        return EAssertDialogResult::Abort;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

