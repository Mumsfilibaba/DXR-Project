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
// CWindowsDebugMisc - Windows-specific interface for debug functions

class CWindowsDebugMisc final : public CPlatformDebugMisc
{
public:

    static FORCEINLINE void DebugBreak()
    {
        __debugbreak();
    }

    static FORCEINLINE void OutputDebugString(const String& Message)
    {
        OutputDebugStringA(Message.CStr());
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }

    static inline void GetLastErrorString(String& OutErrorString)
    {
        int LastError = ::GetLastError();

        const uint32 Flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        LPSTR  MessageBuffer = nullptr;
        uint32 MessageLength = FormatMessageA(Flags, NULL, LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&MessageBuffer, 0, NULL);

        OutErrorString.Clear();
        OutErrorString.Append(MessageBuffer, MessageLength);

        LocalFree(MessageBuffer);
    }
};

#endif