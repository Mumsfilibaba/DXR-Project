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

    /**
     * @brief Read an environment variable of the current process
     *
     * @param Name Name of the variable, which is case-sensitive on Unix and case-insensitive on Windows
     * @param OutValue Receives the value, and is cleared when the variable is not set
     * @return True when the variable is set, including when it is set to an empty value
     */
    static FORCEINLINE bool GetEnvironmentVariable(const CHAR* Name, String& OutValue)
    {
        OutValue.Clear();
        return false;
    }

    /**
     * @brief Set an environment variable of the current process, overwriting any existing value
     *
     * The change is visible to this process and to any child it spawns afterwards, and is lost when the
     * process exits. A null Value removes the variable rather than setting it empty.
     *
     * @param Name Name of the variable
     * @param Value Value to store, or nullptr to remove the variable
     * @return True when the variable was stored or removed
     */
    static FORCEINLINE bool SetEnvironmentVariable(const CHAR* Name, const CHAR* Value)
    {
        return false;
    }

    /** @return Returns how the user chose to respond to a failed assert */
    static FORCEINLINE EAssertDialogResult ShowAssertDialog(const CHAR* Title, const CHAR* Message)
    {
        return EAssertDialogResult::Abort;
    }

    /**
     * @brief Install the process-wide crash handler, which logs a symbolicated report before the process dies.
     *
     * Called once the log file exists and never again, since the second call would have nothing to install
     * over. The report is produced on a thread that did not crash, so it is safe for it to allocate and to
     * take the output device lock.
     */
    static FORCEINLINE void InstallCrashHandler()
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

