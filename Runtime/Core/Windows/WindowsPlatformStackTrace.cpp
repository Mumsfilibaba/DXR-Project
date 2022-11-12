#include "WindowsPlatformStackTrace.h"

#include "Core/Misc/OutputDeviceLogger.h"

#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

static bool GSymbolsInitialized = false;

bool FWindowsPlatformStackTrace::InitializeSymbols()
{
    if (!GSymbolsInitialized)
    {
        CONSTEXPR const uint32 SymOptionFlags = 
            SYMOPT_UNDNAME |
            SYMOPT_FAIL_CRITICAL_ERRORS |
            SYMOPT_DEFERRED_LOADS |
            SYMOPT_LOAD_ANYTHING |
            SYMOPT_EXACT_SYMBOLS |
            SYMOPT_LOAD_LINES;

        uint32 SymOptions = ::SymGetOptions();
        SymOptions |= SymOptionFlags;

        ::SymSetOptions(SymOptions);

        const FString SymbolPath = GetSymbolPath();

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
    SymCleanup(GetCurrentProcess());
    GSymbolsInitialized = false;
}

TArray<FStackTraceEntry> FWindowsPlatformStackTrace::GetStack(int32 MaxDepth)
{
    TArray<FStackTraceEntry> Stack;
    if (!InitializeSymbols())
    {
        return Stack;
    }



    ReleaseSymbols();
    return Stack;
}
