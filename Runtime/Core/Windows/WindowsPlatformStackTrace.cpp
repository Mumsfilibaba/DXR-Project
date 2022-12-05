#include "WindowsPlatformStackTrace.h"

#include "Core/Misc/OutputDeviceLogger.h"

#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

static bool GSymbolsInitialized = false;

bool FWindowsPlatformStackTrace::InitializeSymbols()
{
    if (!GSymbolsInitialized)
    {
        const uint32 SymOptionFlags = 
            SYMOPT_UNDNAME |
            SYMOPT_FAIL_CRITICAL_ERRORS |
            SYMOPT_DEFERRED_LOADS |
            SYMOPT_EXACT_SYMBOLS |
            SYMOPT_LOAD_LINES;

        const uint32 SymOptions = ::SymGetOptions();
        ::SymSetOptions(SymOptions | SymOptionFlags);

        // TODO: Assumed to be the same as the executable path
        const FString SymbolPath = GetSymbolPath();

        // Get handle for the current process handle
        HANDLE CurrentProcess = ::GetCurrentProcess();

        const int32 Result = ::SymInitialize(CurrentProcess, SymbolPath.IsEmpty() ? nullptr : SymbolPath.GetCString(), TRUE);
        if (!Result)
        {
            LOG_ERROR("Failed to initialize Symbols");
        }
        else
        {
            GSymbolsInitialized = true;
        }
    }

    return GSymbolsInitialized;
}

void FWindowsPlatformStackTrace::ReleaseSymbols()
{
    if (GSymbolsInitialized)
    {
        ::SymCleanup(::GetCurrentProcess());
        GSymbolsInitialized = false;
    }
}

int32 FWindowsPlatformStackTrace::CaptureStackTrace(uint64* StackTrace, int32 MaxDepth)
{
    if (!InitializeSymbols())
    {
        return 0;
    }

    CONTEXT Context;
    ::RtlCaptureContext(&Context);

    STACKFRAME64 StackFrame64;
    FMemory::Memzero(&StackFrame64);

    StackFrame64.AddrPC.Mode    = AddrModeFlat;
    StackFrame64.AddrStack.Mode = AddrModeFlat;
    StackFrame64.AddrFrame.Mode = AddrModeFlat;
#if PLATFORM_64BIT
    const uint32 MachineType = IMAGE_FILE_MACHINE_AMD64;
    StackFrame64.AddrPC.Offset    = Context.Rip;
    StackFrame64.AddrStack.Offset = Context.Rsp;
    StackFrame64.AddrFrame.Offset = Context.Rbp;
#else
    const uint32 MachineType = IMAGE_FILE_MACHINE_I386;
    StackFrame64.AddrPC.Offset    = Context.Eip;
    StackFrame64.AddrStack.Offset = Context.Esp;
    StackFrame64.AddrFrame.Offset = Context.Ebp;
#endif

    HANDLE ProcessHandle = ::GetCurrentProcess();
    HANDLE ThreadHandle = ::GetCurrentThread();
    int32  CurrentDepth = 0;
    
    // Reset last error
    ::SetLastError(0);

    bool bStackWalkSucceeded = true;
    while (bStackWalkSucceeded && (CurrentDepth < MaxDepth))
    {
        bStackWalkSucceeded = !!::StackWalk64(
            MachineType,
            ProcessHandle,
            ThreadHandle,
            &StackFrame64,
            &Context,
            nullptr,
            ::SymFunctionTableAccess64,
            ::SymGetModuleBase64,
            nullptr);

        if (!bStackWalkSucceeded)
        {
            break;
        }

        if (StackFrame64.AddrFrame.Offset == 0)
        {
            break;
        }

        StackTrace[CurrentDepth++] = StackFrame64.AddrPC.Offset;
    }

    const int32 Depth = CurrentDepth;
    while (CurrentDepth < MaxDepth)
    {
        StackTrace[CurrentDepth++] = 0;
    }

    return Depth;
}

void FWindowsPlatformStackTrace::GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry)
{
    if (!InitializeSymbols())
    {
        return;
    }

    uint32 LastError = 0;
    HANDLE ProcessHandle = ::GetCurrentProcess();

    // Initialize symbol.
    CHAR SymbolBuffer[sizeof(SYMBOL_INFO) + FStackTraceEntry::MaxNameLength];
    FMemory::Memzero(SymbolBuffer, sizeof(SymbolBuffer));

    SYMBOL_INFO* Symbol = reinterpret_cast<SYMBOL_INFO*>(SymbolBuffer);
    Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    Symbol->MaxNameLen   = FStackTraceEntry::MaxNameLength;

    // Get function name.
    if (::SymFromAddr(ProcessHandle, Address, nullptr, Symbol))
    {
        int32 Offset = 0;
        while (Symbol->Name[Offset] < 32 || Symbol->Name[Offset] > 127)
        {
            Offset++;
        }

        FCString::Strncpy(OutStackTraceEntry.FunctionName, Symbol->Name + Offset, FStackTraceEntry::MaxNameLength);
        FCString::Strncat(OutStackTraceEntry.FunctionName + Offset, "()", FStackTraceEntry::MaxNameLength - Offset);
    }
    else
    {
        // No symbol found for this address.
        LastError = ::GetLastError();
    }

    IMAGEHLP_LINE64	ImageHelpLine;
    FMemory::Memzero(&ImageHelpLine);

    ImageHelpLine.SizeOfStruct = sizeof(ImageHelpLine);

    DWORD SymbolDisplacement;
    if (::SymGetLineFromAddr64(ProcessHandle, Address, &SymbolDisplacement, &ImageHelpLine))
    {
        FCString::Strncpy(OutStackTraceEntry.Filename, ImageHelpLine.FileName, FStackTraceEntry::MaxNameLength);
        OutStackTraceEntry.Line = ImageHelpLine.LineNumber;
    }
    else
    {
        LastError = ::GetLastError();
    }

    IMAGEHLP_MODULE64 ImageHelpModule;
    FMemory::Memzero(&ImageHelpModule);

    ImageHelpModule.SizeOfStruct = sizeof(ImageHelpModule);
    if (::SymGetModuleInfo64(ProcessHandle, Address, &ImageHelpModule))
    {
        FCString::Strncpy(OutStackTraceEntry.ModuleName, ImageHelpModule.ImageName, FStackTraceEntry::MaxNameLength);
    }
    else
    {
        LastError = ::GetLastError();
    }
}

TArray<FStackTraceEntry> FWindowsPlatformStackTrace::GetStack(int32 MaxDepth, int32 IgnoreCount)
{
    TArray<FStackTraceEntry> Stack;
    if (!InitializeSymbols())
    {
        return Stack;
    }

    // Skip the 2 first (CaptureCallstack functions)
    IgnoreCount += 2;

    uint64 StackTrace[MAX_STACK_DEPTH];
    FMemory::Memzero(StackTrace);

    // Ensure that static buffer does not overflow
    MaxDepth = NMath::Min(MAX_STACK_DEPTH, MaxDepth + IgnoreCount);

    const int32 Depth = CaptureStackTrace(StackTrace, MaxDepth);
    for (int32 CurrentDepth = IgnoreCount; CurrentDepth < Depth; CurrentDepth++)
    {
        FStackTraceEntry& NewEntry = Stack.Emplace();
        GetStackTraceEntryFromAddress(StackTrace[CurrentDepth], NewEntry);
    }

    ReleaseSymbols();
    return Stack;
}
