#pragma once

#if PLATFORM_WINDOWS
#include "Windows.h"

#include "Core/Core.h"
#include "Core/Containers/String.h"

#include "CoreApplication/Interface/PlatformDebugMisc.h"

#ifdef MessageBox
    #undef MessageBox
#endif

#ifdef OutputDebugString
    #undef OutputDebugString
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows-specific interface for debug functions

class CWindowsDebugMisc final : public CPlatformDebugMisc
{
public:

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak()
    {
        __debugbreak();
    }

    /* Outputs a debug string to the attached debugger */
    static FORCEINLINE void OutputDebugString(const String& Message)
    {
        OutputDebugStringA(Message.CStr());
    }

    /* Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }

    /* Calls GetLastError and retrieves a string from it */
    static FORCEINLINE void GetLastErrorString(String& OutErrorString)
    {
        DWORD LastError = ::GetLastError();

        const uint32 Flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        LPSTR  MessageBuffer = nullptr;
        uint32 MessageLength = FormatMessageA(Flags, nullptr, LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&MessageBuffer, 0, nullptr);

        OutErrorString.Clear();
        OutErrorString.Append(MessageBuffer, MessageLength);

        LocalFree(MessageBuffer);
    }
};

#endif