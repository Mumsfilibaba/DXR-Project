#pragma once
#include "Windows.h"

#include "Core/Core.h"
#include "Core/Containers/String.h"

#include "CoreApplication/Generic/GenericDebugMisc.h"

#ifdef MessageBox
    #undef MessageBox
#endif

#ifdef OutputDebugString
    #undef OutputDebugString
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsDebugMisc

class CWindowsDebugMisc final : public CGenericDebugMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericDebugMisc Interface

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

public:

    static FORCEINLINE void GetLastErrorString(String& OutErrorString)
    {
        int32 LastError = ::GetLastError();

        const uint32 Flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        LPSTR  MessageBuffer = nullptr;
        uint32 MessageLength = FormatMessageA(Flags, nullptr, LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&MessageBuffer, 0, nullptr);

        OutErrorString.Clear();
        OutErrorString.Append(MessageBuffer, MessageLength);

        LocalFree(MessageBuffer);
    }
};