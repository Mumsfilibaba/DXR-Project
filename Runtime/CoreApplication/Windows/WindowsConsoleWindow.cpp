#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Windows/WindowsConsoleWindow.h"

static FWindowsConsoleWindow* GActiveWindowsConsole = nullptr;

IPlatformConsoleWindow* FWindowsConsoleWindow::Create()
{
    return new FWindowsConsoleWindow();
}

BOOL WINAPI FWindowsConsoleWindow::ConsoleCtrlHandler(DWORD Type)
{
    if ((Type != CTRL_CLOSE_EVENT) && (Type != CTRL_C_EVENT) && (Type != CTRL_BREAK_EVENT))
    {
        return FALSE;
    }

    if (FWindowsConsoleWindow* Console = GActiveWindowsConsole)
    {
        Console->NotifyClosed();
    }

    FOutputDeviceLogger::Get()->Flush();

    ::TerminateProcess(::GetCurrentProcess(), 0);
    return TRUE;
}

FWindowsConsoleWindow::~FWindowsConsoleWindow()
{
    if (GActiveWindowsConsole == this)
    {
        GActiveWindowsConsole = nullptr;
    }

    Show(false);
}

FWindowsConsoleWindow::FWindowsConsoleWindow()
    : ConsoleHandle(0)
{
}

void FWindowsConsoleWindow::SetOnClosed(const TFunction<void()>& Callback)
{
    TScopedLock Lock(ConsoleHandleCS);
    OnClosed = Callback;
}

bool FWindowsConsoleWindow::NotifyClosed()
{
    TFunction<void()> ClosedCopy;
    {
        TScopedLock Lock(ConsoleHandleCS);
        ClosedCopy = OnClosed;
    }

    if (!ClosedCopy)
    {
        return false;
    }

    ClosedCopy();
    return true;
}

void FWindowsConsoleWindow::Show(bool bShow)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (bShow)
    {
        if (!ConsoleHandle)
        {
            ConsoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
            if ((ConsoleHandle == INVALID_HANDLE_VALUE) || !ConsoleHandle)
            {
                ::AllocConsole();
                ConsoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
            }

            if ((ConsoleHandle == INVALID_HANDLE_VALUE) || !ConsoleHandle)
            {
                ConsoleHandle = nullptr;
                LOG_ERROR("[FWindowsConsoleWindow]: Failed to open console. Error: %lu", ::GetLastError());
                return;
            }

            GActiveWindowsConsole = this;

            ::SetConsoleCtrlHandler(&FWindowsConsoleWindow::ConsoleCtrlHandler, TRUE);
            ::SetConsoleTitleA(Title.IsEmpty() ? "Console Output" : *Title);
        }
    }
    else
    {
        if (ConsoleHandle)
        {
            if (GActiveWindowsConsole == this)
            {
                GActiveWindowsConsole = nullptr;
            }

            ConsoleHandle = nullptr;
        }
    }
}

void FWindowsConsoleWindow::SetTitle(const String& InTitle)
{
    TScopedLock Lock(ConsoleHandleCS);

    Title = InTitle;

    if (ConsoleHandle)
    {
        ::SetConsoleTitleA(*InTitle);
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

void FWindowsConsoleWindow::Write(const CHAR* Data, uint32 Length)
{
    DWORD ConsoleMode = 0;
    if (::GetConsoleMode(ConsoleHandle, &ConsoleMode))
    {
        ::WriteConsoleA(ConsoleHandle, Data, Length, nullptr, nullptr);
    }
    else
    {
        ::WriteFile(ConsoleHandle, Data, Length, nullptr, nullptr);
    }
}

void FWindowsConsoleWindow::Log(const String& Message)
{
    TScopedLock Lock(ConsoleHandleCS);

    if (ConsoleHandle)
    {
        Write(*Message, static_cast<uint32>(Message.Length()));
        Write("\n", 1);
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

        Write(*Message, static_cast<uint32>(Message.Length()));
        Write("\n", 1);

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
