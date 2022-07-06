#include "WindowsConsoleWindow.h"

#include "Core/Threading/ScopedLock.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsConsoleWindow

FWindowsConsoleWindow* FWindowsConsoleWindow::CreateWindowsConsole()
{
    return dbg_new FWindowsConsoleWindow();
}

FWindowsConsoleWindow::FWindowsConsoleWindow()
    : ConsoleHandle(0)
{
    if (AllocConsole())
    {
        ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTitleA("Console Output");
    }
}

FWindowsConsoleWindow::~FWindowsConsoleWindow()
{
    if (ConsoleHandle)
    {
        FreeConsole();
        ConsoleHandle = 0;
    }
}

void FWindowsConsoleWindow::Print(const FString& Message)
{
    if (ConsoleHandle)
    {
        TScopedLock<FCriticalSection> Lock(ConsoleHandleCS);
        WriteConsoleA(ConsoleHandle, Message.CStr(), static_cast<DWORD>(Message.Length()), 0, NULL);
    }
}

void FWindowsConsoleWindow::PrintLine(const FString& Message)
{
    if (ConsoleHandle)
    {
        TScopedLock<FCriticalSection> Lock(ConsoleHandleCS);
        WriteConsoleA(ConsoleHandle, Message.CStr(), static_cast<DWORD>(Message.Length()), 0, NULL);
        WriteConsoleA(ConsoleHandle, "\n", 1, 0, NULL);
    }
}

void FWindowsConsoleWindow::Clear()
{
    if (ConsoleHandle)
    {
        TScopedLock<FCriticalSection> Lock(ConsoleHandleCS);

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

void FWindowsConsoleWindow::SetTitle(const FString& Title)
{
    if (ConsoleHandle)
    {
        TScopedLock<FCriticalSection> Lock(ConsoleHandleCS);
        SetConsoleTitleA(Title.CStr());
    }
}

void FWindowsConsoleWindow::SetColor(EConsoleColor Color)
{
    if (ConsoleHandle)
    {
        TScopedLock<FCriticalSection> Lock(ConsoleHandleCS);

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
