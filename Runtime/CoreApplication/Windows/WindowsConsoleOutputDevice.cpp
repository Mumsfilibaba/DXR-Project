#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Windows/WindowsConsoleOutputDevice.h"

FGenericConsoleOutputDevice* FWindowsConsoleOutputDevice::Create()
{
    return new FWindowsConsoleOutputDevice();
}

FWindowsConsoleOutputDevice::FWindowsConsoleOutputDevice()
    : ConsoleHandle(0)
{
}

FWindowsConsoleOutputDevice::~FWindowsConsoleOutputDevice()
{
    Show(false);
}

void FWindowsConsoleOutputDevice::Show(bool bShow)
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

void FWindowsConsoleOutputDevice::Log(const FString& Message)
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);
        WriteConsoleA(ConsoleHandle, *Message, static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);
    }
}

void FWindowsConsoleOutputDevice::Log(ELogSeverity Severity, const FString& Message)
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

        WriteConsoleA(ConsoleHandle, *Message, static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);

        SetTextColor(EConsoleColor::White);
    }
}

void FWindowsConsoleOutputDevice::Flush()
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);

        CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenInfo;
        FMemory::Memzero(&ConsoleScreenInfo);

        if (GetConsoleScreenBufferInfo(ConsoleHandle, &ConsoleScreenInfo))
        {
            COORD     Dest     = { 0, -ConsoleScreenInfo.dwSize.Y };
            CHAR_INFO FillInfo = { '\0', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
            ScrollConsoleScreenBufferA(ConsoleHandle, &ConsoleScreenInfo.srWindow, nullptr, Dest, &FillInfo);

            COORD CursorPos = { 0, 0 };
            SetConsoleCursorPosition(ConsoleHandle, CursorPos);
        }
    }
}

void FWindowsConsoleOutputDevice::SetTitle(const FString& InTitle)
{
    if (ConsoleHandle)
    {
        TScopedLock Lock(ConsoleHandleCS);

        SetConsoleTitleA(*InTitle);
        Title = InTitle;
    }
}

void FWindowsConsoleOutputDevice::SetTextColor(EConsoleColor Color)
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
