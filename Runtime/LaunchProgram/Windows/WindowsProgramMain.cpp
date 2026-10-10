#include "LaunchProgram/ProgramEntry.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/Debug.h"
#include "Core/Windows/Windows.h"
#include "Core/Containers/StringConversion.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static void InitCRunTime()
{
#ifdef DEBUG_BUILD
    uint32 DebugFlags = 0;
    DebugFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(DebugFlags);
#endif
}

static void WaitForKeyWhenConsoleIsOurs()
{
    DWORD ConsoleProcessId = 0;
    if (::GetConsoleProcessList(&ConsoleProcessId, 1) != 1)
    {
        return;
    }

    const WIDECHAR Message[] = L"\nPress any key to exit...\n";
    ::WriteConsoleW(::GetStdHandle(STD_OUTPUT_HANDLE), Message, ARRAY_COUNT(Message) - 1, nullptr, nullptr);

    const HANDLE InputHandle = ::GetStdHandle(STD_INPUT_HANDLE);
    ::FlushConsoleInputBuffer(InputHandle);

    INPUT_RECORD Record;
    DWORD        NumEventsRead = 0;

    while (::ReadConsoleInputW(InputHandle, &Record, 1, &NumEventsRead))
    {
        if ((NumEventsRead > 0) && (Record.EventType == KEY_EVENT) && Record.Event.KeyEvent.bKeyDown)
        {
            break;
        }
    }
}

static int32 RunProgram()
{
    const int32 Result = FProgramLoop::Run(GProgramTitle, GProgramBody);
    WaitForKeyWhenConsoleIsOurs();
    return Result;
}

int wmain(int NumArgs, WIDECHAR** Args)
{
    InitCRunTime();

    TArray<String>      Arguments;
    TArray<const CHAR*> ArgumentPointers;

    for (int32 Index = 1; Index < NumArgs; ++Index)
    {
        Arguments.Add(StringCast<CHAR>(Args[Index]).ToString());
    }

    for (const String& Argument : Arguments)
    {
        ArgumentPointers.Add(*Argument);
    }

    CommandLine::Initialize(ArgumentPointers.Data(), ArgumentPointers.Size());
    return RunProgram();
}

int WINAPI wWinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPWSTR CmdLine, int CmdShow)
{
    InitCRunTime();

    const String CommandLineText  = StringCast<CHAR>(CmdLine).ToString();
    const CHAR*  LocalCommandLine = *CommandLineText;

    CommandLine::Initialize(&LocalCommandLine, 1);
    return RunProgram();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
