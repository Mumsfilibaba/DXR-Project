#include "Core/Misc/Asserts.h"
#include "Core/CoreGlobals.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "Core/Misc/Debug.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Platform/PlatformStackTrace.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/Threading/ScopedLock.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

static constexpr int32 MaxAssertMessageLength = 1024;

static FCriticalSection& GetAssertCriticalSection()
{
    static FCriticalSection AssertCriticalSection;
    return AssertCriticalSection;
}

static uint32 GetAssertHandlingTLSSlot()
{
    static const uint32 AssertHandlingTLSSlot = FPlatformTLS::AllocTLSSlot();
    return AssertHandlingTLSSlot;
}

static bool IsHandlingAssertOnThisThread()
{
    const uint32 SlotIndex = GetAssertHandlingTLSSlot();
    return (SlotIndex != CORE_INVALID_TLS_INDEX) && (FPlatformTLS::GetTLSValue(SlotIndex) != nullptr);
}

static void SetHandlingAssertOnThisThread(bool bIsHandling)
{
    // Checked rather than asserted: FPlatformTLS asserts on a bad slot, which would recurse in here
    const uint32 SlotIndex = GetAssertHandlingTLSSlot();
    if (SlotIndex != CORE_INVALID_TLS_INDEX)
    {
        FPlatformTLS::SetTLSValue(SlotIndex, bIsHandling ? reinterpret_cast<void*>(uintptr_t(1)) : nullptr);
    }
}

/** Sites that have been reported at least once, so repeats can skip the stack trace */
static TSet<uint64>& GetReportedSites()
{
    static TSet<uint64> ReportedSites;
    return ReportedSites;
}

/** Sites the user acknowledged with Ignore, which are downgraded to a log line for the session */
static TSet<uint64>& GetSuppressedSites()
{
    static TSet<uint64> SuppressedSites;
    return SuppressedSites;
}

/** The __FILE__ literal for a given call site always has the same address, so the pointer identifies the file without hashing its contents. */
static uint64 CreateSiteKey(const CHAR* Filename, int32 Line)
{
    const uint64 FileKey = static_cast<uint64>(reinterpret_cast<uintptr_t>(Filename));
    return (FileKey * 1099511628211ull) ^ static_cast<uint64>(Line);
}

/** Terminates the process without letting the platform put another dialog in the way */
[[noreturn]] static void TerminateOnAssert()
{
#if PLATFORM_WINDOWS
    // Neither the CRT abort message nor Windows Error Reporting may appear: an unattended run has to exit, not block.
    ::_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    ::abort();
}

EAssertAction Assert::OnFailed(const CHAR* Expression, const CHAR* Filename, int32 Line, const CHAR* Format, ...)
{
    // An assert raised while reporting an assert cannot be reported. Fail hard instead of recursing.
    if (IsHandlingAssertOnThisThread())
    {
        DEBUG_BREAK();
        TerminateOnAssert();
    }

    SetHandlingAssertOnThisThread(true);

    CHAR Context[MaxAssertMessageLength];
    Context[0] = 0;

    if (Format)
    {
        va_list ArgList;
        va_start(ArgList, Format);
        vsnprintf(Context, sizeof(Context), Format, ArgList);
        va_end(ArgList);
    }

    // Held across the dialog so two threads asserting at once do not stack up two modal windows
    TScopedLock Lock(GetAssertCriticalSection());

    const uint64 SiteKey       = CreateSiteKey(Filename, Line);
    const bool   bIsSuppressed = GetSuppressedSites().Contains(SiteKey);
    const bool   bIsFirstHit   = !GetReportedSites().Contains(SiteKey);

    if (bIsFirstHit)
    {
        GetReportedSites().Add(SiteKey);
    }

    String Record = String::Printf("Assertion failed: %s\n    at %s(%d)", Expression, Filename, Line);
    if (Context[0])
    {
        Record.AppendPrintf("\n    %s", Context);
    }

    // Logged before the dialog goes up so the log has the record even if the user kills the process
    LOG_ERROR("%s", *Record);

    if (bIsFirstHit)
    {
        for (const FStackTraceEntry& Entry : FPlatformStackTrace::GetStack(MAX_STACK_DEPTH, 1))
        {
            LOG_ERROR("    %s (%s:%u)", Entry.FunctionName, Entry.Filename, Entry.Line);
        }
    }

    FOutputDeviceLogger::Get()->Flush();

    SetHandlingAssertOnThisThread(false);

    if (Debug::IsDebuggerPresent())
    {
        return EAssertAction::Break;
    }

    if (IsUnattended())
    {
        // Nobody is there to answer a dialog, so a failed assert has to fail the run
        TerminateOnAssert();
    }

    if (bIsSuppressed)
    {
        return EAssertAction::Continue;
    }

    Record.AppendPrintf("\n\nAbort ends the process, Debug breaks into the debugger, Ignore continues and silences this assertion for the rest of the session.");

    switch (FPlatformMisc::ShowAssertDialog("Assertion Failed", *Record))
    {
        case EAssertDialogResult::Debug:
        {
            return EAssertAction::Break;
        }

        case EAssertDialogResult::Ignore:
        {
            GetSuppressedSites().Add(SiteKey);
            return EAssertAction::Continue;
        }

        default:
        {
            TerminateOnAssert();
        }
    }
}
