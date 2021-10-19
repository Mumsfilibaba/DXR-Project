#include "WindowsOutputConsole.h"

#include "Core/Threading/ScopedLock.h"

CWindowsOutputConsole::CWindowsOutputConsole()
    : ConsoleHandle( 0 )
{
    if ( AllocConsole() )
    {
        ConsoleHandle = GetStdHandle( STD_OUTPUT_HANDLE );
        SetConsoleTitleA( "Console Output" );
    }
}

CWindowsOutputConsole::~CWindowsOutputConsole()
{
    if ( ConsoleHandle )
    {
        FreeConsole();
        ConsoleHandle = 0;
    }
}

CWindowsOutputConsole* CWindowsOutputConsole::Make()
{
    return dbg_new CWindowsOutputConsole();
}

void CWindowsOutputConsole::Print( const CString& Message )
{
    if ( ConsoleHandle )
    {
        TScopedLock<CCriticalSection> Lock( ConsoleMutex );
        WriteConsoleA( ConsoleHandle, Message.CStr(), static_cast<DWORD>(Message.Length()), 0, NULL );
    }
}

void CWindowsOutputConsole::PrintLine( const CString& Message )
{
    UNREFERENCED_VARIABLE( Message );
}

void CWindowsOutputConsole::Clear()
{
    if ( ConsoleHandle )
    {
        TScopedLock<CCriticalSection> Lock( ConsoleMutex );

        CONSOLE_SCREEN_BUFFER_INFO CSBI;
        CMemory::Memzero( &CSBI );

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

void CWindowsOutputConsole::SetTitle( const CString& Title )
{
    if ( ConsoleHandle )
    {
        TScopedLock<CCriticalSection> Lock( ConsoleMutex );
        SetConsoleTitleA( Title.CStr() );
    }
}

void CWindowsOutputConsole::SetColor( EConsoleColor Color )
{
    if ( ConsoleHandle )
    {
        TScopedLock<CCriticalSection> Lock( ConsoleMutex );

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
