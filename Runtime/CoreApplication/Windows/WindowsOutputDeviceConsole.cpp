#include "WindowsOutputDeviceConsole.h"

#include "Core/Threading/ScopedLock.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsOutputDeviceConsole

FWindowsOutputDeviceConsole::FWindowsOutputDeviceConsole()
    : ConsoleHandle(0)
{ }

FWindowsOutputDeviceConsole::~FWindowsOutputDeviceConsole()
{
    Show(false);
}

void FWindowsOutputDeviceConsole::Show(bool bShow)
{
    if (bShow)
    {
        TScopedLock Lock(ConsoleHandleCS);
        if (!ConsoleHandle)
        {
            if (AllocConsole())
            {
                ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTitleA("Console Output");
            }
        }
    }
    else if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);
        FreeConsole();
        ConsoleHandle = 0;
    }
}

void FWindowsOutputDeviceConsole::Log(const FString& Message)
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);
        WriteConsoleA(ConsoleHandle, Message.CStr(), static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);
    }
}

void FWindowsOutputDeviceConsole::Log(ELogSeverity Severity, const FString& Message)
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);

        EConsoleColor NewColor;
        if (Severity == ELogSeverity::Info)
        {
            NewColor = EConsoleColor::Green;
        }
        else if (Severity == ELogSeverity::Warning)
        {
            NewColor = EConsoleColor::Yellow;
        }
        else if (Severity == ELogSeverity::Error)
        {
            NewColor = EConsoleColor::Red;
        }
        else
        {
            NewColor = EConsoleColor::White;
        }
        
        SetTextColor(NewColor);

        WriteConsoleA(ConsoleHandle, Message.CStr(), static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);

        SetTextColor(EConsoleColor::White);
    }
}

void FWindowsOutputDeviceConsole::Flush()
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);

        CONSOLE_SCREEN_BUFFER_INFO CSBI;
        FMemory::Memzero(&CSBI);

        if (GetConsoleScreenBufferInfo(ConsoleHandle, &CSBI))
        {
            COORD     Dest     = { 0, -CSBI.dwSize.Y };
            CHAR_INFO FillInfo = { '\0', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
            ScrollConsoleScreenBufferA(ConsoleHandle, &CSBI.srWindow, nullptr, Dest, &FillInfo);

            COORD CursorPos = { 0, 0 };
            SetConsoleCursorPosition(ConsoleHandle, CursorPos);
        }
    }
}

void FWindowsOutputDeviceConsole::SetTitle(const FString& InTitle)
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);

        SetConsoleTitleA(InTitle.CStr());
        Title = InTitle;
    }
}

void FWindowsOutputDeviceConsole::SetTextColor(EConsoleColor Color)
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);

        WORD wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        switch (Color)
        {
        case EConsoleColor::Red:
            wColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;

        case EConsoleColor::Green:
            wColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;

        case EConsoleColor::Yellow:
            wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;

        case EConsoleColor::White:
            break;
        }

        SetConsoleTextAttribute(ConsoleHandle, wColor);
    }
}
