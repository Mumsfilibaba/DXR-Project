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
    Show(false); // Hides/frees console if open
}

void FWindowsConsoleOutputDevice::Show(bool bShow)
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

                // If you want to preserve previously set Title, use:
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

void FWindowsConsoleOutputDevice::Log(const FString& Message)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        // Convert to an ANSI string if needed
        WriteConsoleA(ConsoleHandle, *Message, static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);
    }
}

void FWindowsConsoleOutputDevice::Log(ELogSeverity Severity, const FString& Message)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        EConsoleColor NewColor;
        switch (Severity)
        {
        case ELogSeverity::Info:
            NewColor = EConsoleColor::Green;
            break;
        case ELogSeverity::Warning:
            NewColor = EConsoleColor::Yellow;
            break;
        case ELogSeverity::Error:
            NewColor = EConsoleColor::Red;
            break;
        default:
            NewColor = EConsoleColor::White;
            break;
        }

        SetTextColor(NewColor);

        // Convert to an ANSI string if needed
        WriteConsoleA(ConsoleHandle, *Message, static_cast<DWORD>(Message.Length()), nullptr, nullptr);
        WriteConsoleA(ConsoleHandle, "\n", 1, nullptr, nullptr);

        SetTextColor(EConsoleColor::White);
    }
}

void FWindowsConsoleOutputDevice::Flush()
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenInfo;
        FMemory::Memzero(&ConsoleScreenInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));

        if (::GetConsoleScreenBufferInfo(ConsoleHandle, &ConsoleScreenInfo))
        {
            // Scroll the console window up by the entire window height
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

void FWindowsConsoleOutputDevice::SetTitle(const FString& InTitle)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        ::SetConsoleTitleA(*InTitle);
        Title = InTitle;
    }
    else
    {
        // Could store the title for future usage
        Title = InTitle;
    }
}

void FWindowsConsoleOutputDevice::SetTextColor(EConsoleColor Color)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        WORD wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // White
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
        default:
            // wColor stays as white
            break;
        }

        ::SetConsoleTextAttribute(ConsoleHandle, wColor);
    }
}
