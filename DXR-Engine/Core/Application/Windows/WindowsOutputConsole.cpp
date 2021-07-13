#include "WindowsOutputConsole.h"

#include "Core/Threading/ScopedLock.h"

GenericOutputConsole* GenericOutputConsole::Create()
{
    return DBG_NEW WindowsOutputConsole();
}

WindowsOutputConsole::WindowsOutputConsole()
    : ConsoleHandle( 0 )
{
    if ( AllocConsole() )
    {
        ConsoleHandle = GetStdHandle( STD_OUTPUT_HANDLE );
        SetConsoleTitleA( "Console Output" );
    }
}

WindowsOutputConsole::~WindowsOutputConsole()
{
    if ( ConsoleHandle )
    {
        FreeConsole();
        ConsoleHandle = 0;
    }
}

void WindowsOutputConsole::Print( const std::string& Message )
{
    if ( ConsoleHandle )
    {
        TScopedLock<Mutex> Lock( ConsoleMutex );

        WriteConsoleA( ConsoleHandle, Message.c_str(), static_cast<DWORD>(Message.size()), 0, NULL );
    }
}

void WindowsOutputConsole::Clear()
{
    if ( ConsoleHandle )
    {
        TScopedLock<Mutex> Lock( ConsoleMutex );

        CONSOLE_SCREEN_BUFFER_INFO CSBI;
        Memory::Memzero( &CSBI );

        if ( GetConsoleScreenBufferInfo( ConsoleHandle, &CSBI ) )
        {
            COORD     Dest = { 0, -CSBI.dwSize.Y };
            CHAR_INFO FillInfo = { '\0', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
            ScrollConsoleScreenBufferA( ConsoleHandle, &CSBI.srWindow, nullptr, Dest, &FillInfo );

            COORD CursorPos = { 0, 0 };
            SetConsoleCursorPosition( ConsoleHandle, CursorPos );
        }
    }
}

void WindowsOutputConsole::SetTitle( const std::string& Title )
{
    if ( ConsoleHandle )
    {
        TScopedLock<Mutex> Lock( ConsoleMutex );

        SetConsoleTitleA( Title.c_str() );
    }
}

void WindowsOutputConsole::SetColor( EConsoleColor Color )
{
    if ( ConsoleHandle )
    {
        TScopedLock<Mutex> Lock( ConsoleMutex );

        WORD wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        switch ( Color )
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

        SetConsoleTextAttribute( ConsoleHandle, wColor );
    }
}
