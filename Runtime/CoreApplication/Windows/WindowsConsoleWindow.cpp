#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Windows/WindowsConsoleWindow.h"

IPlatformConsoleWindow* FWindowsConsoleWindow::Create()
{
    return new FWindowsConsoleWindow();
}

FWindowsConsoleWindow::~FWindowsConsoleWindow()
{
    Show(false);
}

FWindowsConsoleWindow::FWindowsConsoleWindow()
    : ConsoleHandle(0)
{
}

void FWindowsConsoleWindow::Show(bool bShow)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (bShow)
    {
        if (!ConsoleHandle)
        {
            if (AllocConsole())
            {
                ConsoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
                if (!ConsoleHandle)
                {
                    // TODO: log this error
                }

                if (!Title.IsEmpty())
                {
                    SetConsoleTitleA(*Title);
                }
                else
                {
                    SetConsoleTitleA("Console Output");
                }
            }
            else
            {
                // TODO: Log error: AllocConsole() failed
            }
        }
    }
    else
    {
        if (ConsoleHandle)
        {
            FreeConsole();
            ConsoleHandle = nullptr;
        }
    }
}

void FWindowsConsoleWindow::SetTitle(const String& InTitle)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        ::SetConsoleTitleA(*InTitle);
        Title = InTitle;
    }
    else
    {
        Title = InTitle;
    }
}

void FWindowsConsoleWindow::SetTextColor(EConsoleTextColor Color)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        WORD wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        switch (Color)
        {
        case EConsoleTextColor::Red:
            wColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;

        case EConsoleTextColor::Green:
            wColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;

        case EConsoleTextColor::Yellow:
            wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;

        case EConsoleTextColor::White:
        default:
            break;
        }

        ::SetConsoleTextAttribute(ConsoleHandle, wColor);
    }
}

void FWindowsConsoleWindow::Log(const String& Message)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        WriteConsoleA(ConsoleHandle, *Message, static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);
    }
}

void FWindowsConsoleWindow::Log(ELogSeverity Severity, const String& Message)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        EConsoleTextColor NewColor;
        switch (Severity)
        {
        case ELogSeverity::Info:
            NewColor = EConsoleTextColor::Green;
            break;
        case ELogSeverity::Warning:
            NewColor = EConsoleTextColor::Yellow;
            break;
        case ELogSeverity::Error:
            NewColor = EConsoleTextColor::Red;
            break;
        default:
            NewColor = EConsoleTextColor::White;
            break;
        }

        SetTextColor(NewColor);

        WriteConsoleA(ConsoleHandle, *Message, static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);

        SetTextColor(EConsoleTextColor::White);
    }
}

void FWindowsConsoleWindow::Flush()
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenInfo;
        Memory::Memzero(&ConsoleScreenInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));

        if (::GetConsoleScreenBufferInfo(ConsoleHandle, &ConsoleScreenInfo))
        {
            COORD Dest = {0, static_cast<SHORT>(-ConsoleScreenInfo.dwSize.Y)};
            CHAR_INFO FillInfo = {' ', static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)};

            ::ScrollConsoleScreenBufferA(ConsoleHandle, &ConsoleScreenInfo.srWindow, nullptr, Dest, &FillInfo);

            COORD CursorPos = {0, 0};
            ::SetConsoleCursorPosition(ConsoleHandle, CursorPos);
        }
        else
        {
            // TODO: Log GetConsoleScreenBufferInfo failed
        }
    }
}
