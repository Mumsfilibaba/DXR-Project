#include "Core/Windows/WindowsPlatformStackTrace.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Math/Math.h"
#include <dbghelp.h>

static FCriticalSection GSymbolsCS;
static int32            GSymbolsRefCount = 0;

static void CopyEntryName(CHAR_T* Dest, const WIDECHAR* Source)
{
    const int32 SourceLength = static_cast<int32>(CStringWide::Strlen(Source));

#if WIDE_CHAR_BUILD
    const int32 Length = Math::Min(SourceLength, static_cast<int32>(FStackTraceEntry::MaxNameLength) - 1);
    CStringWide::Strncpy(Dest, Source, Length);
#else
    const int32 Length = FStringConversion::Convert(reinterpret_cast<UTF8CHAR*>(Dest), FStackTraceEntry::MaxNameLength - 1, reinterpret_cast<const char16_t*>(Source), SourceLength);
#endif

    Dest[Length] = 0;
}

bool FWindowsPlatformStackTrace::InitializeSymbols()
{
    TScopedLock Lock(GSymbolsCS);

    if (GSymbolsRefCount == 0)
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

        const TStringConversion<WIDECHAR, CHAR_T> WideSymbolPath = StringCast<WIDECHAR>(SymbolPath);

        const int32 Result = ::SymInitializeW(CurrentProcess, SymbolPath.IsEmpty() ? nullptr : WideSymbolPath.Get(), TRUE);
        if (!Result)
        {
            LOG_ERROR("Failed to initialize Symbols");
            return false;
        }
    }

    ++GSymbolsRefCount;
    return true;
}

void FWindowsPlatformStackTrace::ReleaseSymbols()
{
    TScopedLock Lock(GSymbolsCS);

    if ((GSymbolsRefCount > 0) && (--GSymbolsRefCount == 0))
    {
        ::SymCleanup(::GetCurrentProcess());
    }
}

// StackWalk64 advances the context it is given, so the caller has to hand over a copy it can afford to lose
static int32 WalkStack(HANDLE ThreadHandle, CONTEXT& Context, uint64* StackTrace, int32 MaxDepth)
{
    STACKFRAME64 StackFrame64;
    Memory::Memzero(&StackFrame64);

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
    int32  CurrentDepth  = 0;

    // Reset last error
    ::SetLastError(0);

    bool bStackWalkSucceeded = true;
    while (bStackWalkSucceeded && (CurrentDepth < MaxDepth))
    {
        bStackWalkSucceeded = ::StackWalk64(MachineType, ProcessHandle, ThreadHandle, &StackFrame64, &Context, nullptr, ::SymFunctionTableAccess64, ::SymGetModuleBase64, nullptr);
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

int32 FWindowsPlatformStackTrace::CaptureStackTrace(uint64* StackTrace, int32 MaxDepth)
{
    if (!InitializeSymbols())
    {
        return 0;
    }

    CONTEXT Context;
    ::RtlCaptureContext(&Context);

    const int32 Depth = WalkStack(::GetCurrentThread(), Context, StackTrace, MaxDepth);

    ReleaseSymbols();
    return Depth;
}

int32 FWindowsPlatformStackTrace::CaptureThreadStackTrace(const FThreadStackContext& ThreadContext, uint64* StackTrace, int32 MaxDepth)
{
    if (!ThreadContext.ThreadHandle || !ThreadContext.RegisterState)
    {
        return 0;
    }

    if (!InitializeSymbols())
    {
        return 0;
    }

    CONTEXT Context = *reinterpret_cast<const CONTEXT*>(ThreadContext.RegisterState);
    const int32 Depth = WalkStack(reinterpret_cast<HANDLE>(ThreadContext.ThreadHandle), Context, StackTrace, MaxDepth);

    ReleaseSymbols();
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
    alignas(SYMBOL_INFOW) uint8 SymbolBuffer[sizeof(SYMBOL_INFOW) + FStackTraceEntry::MaxNameLength * sizeof(WIDECHAR)];
    Memory::Memzero(SymbolBuffer, sizeof(SymbolBuffer));

    SYMBOL_INFOW* Symbol = reinterpret_cast<SYMBOL_INFOW*>(SymbolBuffer);
    Symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
    Symbol->MaxNameLen   = FStackTraceEntry::MaxNameLength;

    if (::SymFromAddrW(ProcessHandle, Address, nullptr, Symbol))
    {
        int32 Offset = 0;

        // NOTE: Allowed characters in the Ascii-table
        while ((Symbol->Name[Offset] != 0) && ((Symbol->Name[Offset] < ' ') || (Symbol->Name[Offset] > 127)))
        {
            Offset++;
        }

        CopyEntryName(OutStackTraceEntry.FunctionName, Symbol->Name + Offset);
        const int32 NameLength = static_cast<int32>(CString::Strlen(OutStackTraceEntry.FunctionName));
        CString::Strncat(OutStackTraceEntry.FunctionName, TEXT("()"), FStackTraceEntry::MaxNameLength - NameLength - 1);
    }
    else
    {
        // No symbol found for this address.
        LastError = ::GetLastError();
    }

    IMAGEHLP_LINEW64 ImageHelpLine;
    Memory::Memzero(&ImageHelpLine);

    ImageHelpLine.SizeOfStruct = sizeof(ImageHelpLine);

    DWORD SymbolDisplacement;
    if (::SymGetLineFromAddrW64(ProcessHandle, Address, &SymbolDisplacement, &ImageHelpLine))
    {
        CopyEntryName(OutStackTraceEntry.Filename, ImageHelpLine.FileName);
        OutStackTraceEntry.Line = ImageHelpLine.LineNumber;
    }
    else
    {
        LastError = ::GetLastError();
    }

    IMAGEHLP_MODULEW64 ImageHelpModule;
    Memory::Memzero(&ImageHelpModule);

    ImageHelpModule.SizeOfStruct = sizeof(ImageHelpModule);
    if (::SymGetModuleInfoW64(ProcessHandle, Address, &ImageHelpModule))
    {
        CopyEntryName(OutStackTraceEntry.ModuleName, ImageHelpModule.ImageName);
    }
    else
    {
        LastError = ::GetLastError();
    }

    ReleaseSymbols();
}
