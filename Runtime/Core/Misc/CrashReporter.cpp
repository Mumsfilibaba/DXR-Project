#include "Core/Misc/CrashReporter.h"
#include "Core/Containers/String.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformStackTrace.h"

void CrashReporter::Report(const FCrashContext& Context)
{
    const CHAR* ExceptionName = Context.ExceptionName ? Context.ExceptionName : "Unknown";
    const CHAR* ThreadName    = (Context.ThreadName && Context.ThreadName[0]) ? Context.ThreadName : "Unnamed";

    String Record = String::Printf("Fatal error: %s (code 0x%llX) on thread '%s'", ExceptionName, Context.ExceptionCode, ThreadName);
    Record.AppendPrintf("\n    Faulting address: 0x%016llX", Context.FaultAddress);

    const TArray<FStackTraceEntry> Stack = FPlatformStackTrace::GetThreadStack(Context.ThreadStack, MAX_STACK_DEPTH);
    for (int32 Index = 0; Index < Stack.Size(); Index++)
    {
        const FStackTraceEntry& Entry = Stack[Index];
        if (Entry.Filename[0])
        {
            Record.AppendPrintf("\n    [%2d] %s (%s:%u)", Index, Entry.FunctionName, Entry.Filename, Entry.Line);
        }
        else
        {
            Record.AppendPrintf("\n    [%2d] %s [%s]", Index, Entry.FunctionName, Entry.ModuleName);
        }
    }

    if (Stack.IsEmpty())
    {
        Record.Append("\n    No call-stack could be walked for the faulting thread");
    }

    LOG_ERROR("%s", *Record);

    // The process is about to die, so the report is worth nothing unless it has reached the file.
    FOutputDeviceLogger::Get()->Flush();
}
