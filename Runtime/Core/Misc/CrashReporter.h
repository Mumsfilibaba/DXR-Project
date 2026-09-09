#pragma once
#include "Core/Core.h"
#include "Core/PlatformInterface/IPlatformStackTrace.h"

struct FCrashContext
{
    /** @brief The fault as the platform spells it, "EXC_BAD_ACCESS" or "EXCEPTION_ACCESS_VIOLATION". */
    const CHAR* ExceptionName = nullptr;

    /** @brief The platform's numeric code for the fault, a Mach exception code or a Windows exception code. */
    uint64 ExceptionCode = 0;

    /** @brief The address the fault refers to, or zero when the platform reports none. */
    uint64 FaultAddress = 0;

    /** @brief Name of the thread that faulted, null or empty when the OS carries none for it. */
    const CHAR* ThreadName = nullptr;

    /** @brief Identifies the faulting thread so the reporting thread can walk its stack. */
    FThreadStackContext ThreadStack;
};

struct CORE_API CrashReporter
{
    /**
     * @brief Writes the fault and the faulting thread's call-stack to the log, then flushes it.
     *
     * Has to run on a thread that did not crash and while the faulting thread is stopped, because it takes
     * the output device lock and allocates, neither of which survives a corrupted stack.
     *
     * @param Context What the platform handler knows about the fault.
     */
    static void Report(const FCrashContext& Context);
};
