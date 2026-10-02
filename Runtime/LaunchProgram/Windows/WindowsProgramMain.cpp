#include "LaunchProgram/ProgramEntry.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/Debug.h"
#include "Core/Windows/Windows.h"

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

    const CHAR Message[] = "\nPress any key to exit...\n";
    ::WriteConsoleA(::GetStdHandle(STD_OUTPUT_HANDLE), Message, ARRAY_COUNT(Message) - 1, nullptr, nullptr);

    const HANDLE InputHandle = ::GetStdHandle(STD_INPUT_HANDLE);
    ::FlushConsoleInputBuffer(InputHandle);

    INPUT_RECORD Record;
    DWORD        NumEventsRead = 0;

    while (::ReadConsoleInputA(InputHandle, &Record, 1, &NumEventsRead))
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

int main(int NumArgs, const CHAR** Args)
{
    InitCRunTime();

    CommandLine::Initialize(Args + 1, NumArgs - 1);
    return RunProgram();
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    InitCRunTime();

    const CHAR* LocalCommandLine = CmdLine;
    CommandLine::Initialize(&LocalCommandLine, 1);
    return RunProgram();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
