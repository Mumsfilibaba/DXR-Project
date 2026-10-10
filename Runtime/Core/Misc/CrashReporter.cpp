#include "Core/Misc/CrashReporter.h"
#include "Core/Containers/String.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformStackTrace.h"

void CrashReporter::Report(const FCrashContext& Context)
{
    const CHAR_T* ExceptionName = Context.ExceptionName ? Context.ExceptionName : TEXT("Unknown");
    const CHAR_T* ThreadName    = (Context.ThreadName && Context.ThreadName[0]) ? Context.ThreadName : TEXT("Unnamed");

    FString Record = FString::Printf(TEXT("Fatal error: %s (code 0x%llX) on thread '%s'"), ExceptionName, Context.ExceptionCode, ThreadName);
    Record.AppendPrintf(TEXT("\n    Faulting address: 0x%016llX"), Context.FaultAddress);

    const TArray<FStackTraceEntry> Stack = FPlatformStackTrace::GetThreadStack(Context.ThreadStack, MAX_STACK_DEPTH);
    for (int32 Index = 0; Index < Stack.Size(); Index++)
    {
        const FStackTraceEntry& Entry = Stack[Index];
        if (Entry.Filename[0])
        {
            Record.AppendPrintf(TEXT("\n    [%2d] %s (%s:%u)"), Index, Entry.FunctionName, Entry.Filename, Entry.Line);
        }
        else
        {
            Record.AppendPrintf(TEXT("\n    [%2d] %s [%s]"), Index, Entry.FunctionName, Entry.ModuleName);
        }
    }

    if (Stack.IsEmpty())
    {
        Record.Append(TEXT("\n    No call-stack could be walked for the faulting thread"));
    }

    LOG_ERROR("%s", *Record);

    // The process is about to die, so the report is worth nothing unless it has reached the file.
    FOutputDeviceManager::Get()->Flush();
}
