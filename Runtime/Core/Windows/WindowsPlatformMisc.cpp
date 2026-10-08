#include "Core/Windows/WindowsPlatformMisc.h"
#include "Core/Windows/WindowsPlatformStackTrace.h"
#include "Core/Misc/CrashReporter.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformAtomic.h"

#include <eh.h>
#include <intrin.h>
#include <stdlib.h>

// Not in winnt.h, which only carries the codes the SEH macros name
constexpr uint32 EXCEPTION_CODE_HEAP_CORRUPTION      = 0xC0000374;
constexpr uint32 EXCEPTION_CODE_STACK_BUFFER_OVERRUN = 0xC0000409;

/** How long the faulting thread waits for its report before giving up and letting the process die */
constexpr uint32 REPORT_TIMEOUT_MILLISECONDS = 10 * 1000;

/** Headroom left below the guard page so a stack-overflow exception still has room to run the handler */
constexpr uint32 STACK_GUARANTEE_BYTES = 64 * 1024;

// Created before anything can crash, because a handler cannot afford to create them
static HANDLE GReportRequestedEvent = nullptr;
static HANDLE GReportCompleteEvent  = nullptr;

// Published by the faulting thread and read by the dump thread, which the events order
static EXCEPTION_POINTERS* GExceptionPointers = nullptr;
static HANDLE              GFaultingThread    = nullptr;
static const CHAR*         GOverriddenName    = nullptr;

// Claimed by whichever thread crashes first, so a fault raised while reporting cannot report again
static volatile int32 GIsReporting = 0;

static const CHAR* GetExceptionName(uint32 ExceptionCode)
{
    switch (ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
            return "EXCEPTION_ACCESS_VIOLATION";

        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";

        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "EXCEPTION_DATATYPE_MISALIGNMENT";

        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "EXCEPTION_FLT_DIVIDE_BY_ZERO";

        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "EXCEPTION_ILLEGAL_INSTRUCTION";

        case EXCEPTION_IN_PAGE_ERROR:
            return "EXCEPTION_IN_PAGE_ERROR";

        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "EXCEPTION_INT_DIVIDE_BY_ZERO";

        case EXCEPTION_INVALID_DISPOSITION:
            return "EXCEPTION_INVALID_DISPOSITION";

        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "EXCEPTION_NONCONTINUABLE_EXCEPTION";

        case EXCEPTION_PRIV_INSTRUCTION:
            return "EXCEPTION_PRIV_INSTRUCTION";

        case EXCEPTION_STACK_OVERFLOW:
            return "EXCEPTION_STACK_OVERFLOW";

        case EXCEPTION_CODE_HEAP_CORRUPTION:
            return "STATUS_HEAP_CORRUPTION";

        case EXCEPTION_CODE_STACK_BUFFER_OVERRUN:
            return "STATUS_STACK_BUFFER_OVERRUN";

        default:
            return "EXCEPTION_UNKNOWN";
    }
}

static bool IsFatalException(uint32 ExceptionCode)
{
    switch (ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_INVALID_DISPOSITION:
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_CODE_HEAP_CORRUPTION:
        case EXCEPTION_CODE_STACK_BUFFER_OVERRUN:
            return true;

        default:
            return false;
    }
}

static uint64 GetFaultAddress(const EXCEPTION_RECORD& ExceptionRecord)
{
    const bool bHasTouchedAddress =
        (ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) ||
        (ExceptionRecord.ExceptionCode == EXCEPTION_IN_PAGE_ERROR);

    if (bHasTouchedAddress && (ExceptionRecord.NumberParameters >= 2))
    {
        return static_cast<uint64>(ExceptionRecord.ExceptionInformation[1]);
    }

    return static_cast<uint64>(reinterpret_cast<uintptr_t>(ExceptionRecord.ExceptionAddress));
}

static DWORD WINAPI CrashReportThreadEntry(LPVOID)
{
    ::SetThreadDescription(::GetCurrentThread(), L"CrashHandler");
    ::WaitForSingleObject(GReportRequestedEvent, INFINITE);

    const EXCEPTION_RECORD& ExceptionRecord = *GExceptionPointers->ExceptionRecord;

    // Resolved here rather than in the handler, because it allocates and the faulting thread must not.
    String ThreadName;
    PWSTR  Description = nullptr;
    if (SUCCEEDED(::GetThreadDescription(GFaultingThread, &Description)))
    {
        ThreadName = WideToChar(WString(Description));
        ::LocalFree(Description);
    }

    FCrashContext Context;
    Context.ExceptionName             = GOverriddenName ? GOverriddenName : GetExceptionName(ExceptionRecord.ExceptionCode);
    Context.ExceptionCode             = static_cast<uint64>(ExceptionRecord.ExceptionCode);
    Context.FaultAddress              = GetFaultAddress(ExceptionRecord);
    Context.ThreadName                = *ThreadName;
    Context.ThreadStack.ThreadHandle  = static_cast<uint64>(reinterpret_cast<uintptr_t>(GFaultingThread));
    Context.ThreadStack.RegisterState = GExceptionPointers->ContextRecord;

    CrashReporter::Report(Context);

    ::SetEvent(GReportCompleteEvent);
    return 0;
}

static void RequestCrashReport(EXCEPTION_POINTERS* ExceptionPointers, const CHAR* OverriddenName)
{
    if (!GReportRequestedEvent || !GReportCompleteEvent)
    {
        return;
    }

    if (FPlatformAtomic::InterlockedCompareExchange(&GIsReporting, 1, 0) != 0)
    {
        return;
    }

    ::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &GFaultingThread, 0, FALSE, DUPLICATE_SAME_ACCESS);

    GExceptionPointers = ExceptionPointers;
    GOverriddenName    = OverriddenName;

    ::SetEvent(GReportRequestedEvent);
    ::WaitForSingleObject(GReportCompleteEvent, REPORT_TIMEOUT_MILLISECONDS);
}

static void ReportWithoutException(const CHAR* Reason)
{
    static CONTEXT            SyntheticContext;
    static EXCEPTION_RECORD   SyntheticRecord;
    static EXCEPTION_POINTERS SyntheticPointers;

    Memory::Memzero(&SyntheticContext);
    Memory::Memzero(&SyntheticRecord);

    ::RtlCaptureContext(&SyntheticContext);

    SyntheticRecord.ExceptionCode     = EXCEPTION_NONCONTINUABLE_EXCEPTION;
    SyntheticRecord.ExceptionFlags    = EXCEPTION_NONCONTINUABLE;
    SyntheticRecord.ExceptionAddress  = _ReturnAddress();
    SyntheticPointers.ExceptionRecord = &SyntheticRecord;
    SyntheticPointers.ContextRecord   = &SyntheticContext;

    RequestCrashReport(&SyntheticPointers, Reason);
}

static LONG CALLBACK HandleVectoredException(EXCEPTION_POINTERS* ExceptionPointers)
{
    if (ExceptionPointers && ExceptionPointers->ExceptionRecord && IsFatalException(ExceptionPointers->ExceptionRecord->ExceptionCode))
    {
        RequestCrashReport(ExceptionPointers, nullptr);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI HandleUnhandledException(EXCEPTION_POINTERS* ExceptionPointers)
{
    if (ExceptionPointers && ExceptionPointers->ExceptionRecord)
    {
        RequestCrashReport(ExceptionPointers, nullptr);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static void HandlePureCall()
{
    ReportWithoutException("Pure virtual function call");
    ::abort();
}

static void HandleInvalidParameter(const WIDECHAR*, const WIDECHAR*, const WIDECHAR*, unsigned int, uintptr_t)
{
    ReportWithoutException("Invalid parameter passed to a CRT function");
    ::abort();
}

static void HandleTerminate()
{
    ReportWithoutException("std::terminate was called");
    ::abort();
}

void FWindowsPlatformMisc::InstallCrashHandler()
{
    static bool bIsInstalled = false;
    if (bIsInstalled)
    {
        return;
    }

    bIsInstalled = true;

    // Held for the life of the process, so the crash path never has to call SymInitialize while the process is dying.
    FWindowsPlatformStackTrace::InitializeSymbols();

    // A stack-overflow exception is delivered on the stack that overflowed
    // so the handler needs room left over below the guard page to run in at all.
    ULONG StackGuarantee = STACK_GUARANTEE_BYTES;
    ::SetThreadStackGuarantee(&StackGuarantee);

    GReportRequestedEvent = ::CreateEventA(nullptr, FALSE, FALSE, nullptr);
    GReportCompleteEvent  = ::CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (!GReportRequestedEvent || !GReportCompleteEvent)
    {
        LOG_ERROR("Failed to create the crash handler's events");
        return;
    }

    HANDLE ReportThread = ::CreateThread(nullptr, 0, CrashReportThreadEntry, nullptr, 0, nullptr);
    if (!ReportThread)
    {
        LOG_ERROR("Failed to start the crash handler thread");
        return;
    }

    // Nothing ever joins it, so the handle goes back now and the thread runs until the process does not.
    ::CloseHandle(ReportThread);

    // Vectored rather than only unhandled, because it runs before any frame-based __except and so cannot be
    // swallowed upstream, and because the CRT is free to replace the unhandled filter.
    ::AddVectoredExceptionHandler(1, HandleVectoredException);
    ::SetUnhandledExceptionFilter(HandleUnhandledException);

    // The three ways the CRT ends a process without ever raising an exception.
    ::_set_purecall_handler(HandlePureCall);
    ::_set_invalid_parameter_handler(HandleInvalidParameter);
    ::set_terminate(&HandleTerminate);
}
