#if defined(PLATFORM_WINDOWS)
#include "WindowsDebugMisc.h"

void CWindowsDebugMisc::GetLastErrorString( CString& OutErrorString )
{
    int LastError = ::GetLastError();

    const uint32 Flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    LPSTR  MessageBuffer = nullptr;
    uint32 MessageLength = FormatMessageA( Flags, NULL, LastError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&MessageBuffer, 0, NULL );

    OutErrorString.Clear();
    OutErrorString.Append( MessageBuffer, MessageLength );

    LocalFree( MessageBuffer );
}

#endif