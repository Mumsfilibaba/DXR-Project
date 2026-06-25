#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/String.h"
#include "Core/Generic/GenericPlatformMisc.h"

struct FWindowsPlatformMisc final : public FGenericPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR* Message)
    {
        ::OutputDebugStringA(Message);
        // Add a new line here since that makes this function consistent with OutputDebugString on other platforms
        ::OutputDebugStringA("\n");
    }

    static FORCEINLINE void WriteToStdOutput(const CHAR* Text, uint32 Length)
    {
        if (Length == 0)
        {
            return;
        }

        HANDLE Handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (Handle && (Handle != INVALID_HANDLE_VALUE))
        {
            DWORD Written = 0;
            // Console handles need WriteConsoleA, but redirected handles (files/pipes) require WriteFile
            if (::GetFileType(Handle) == FILE_TYPE_CHAR)
            {
                ::WriteConsoleA(Handle, Text, Length, &Written, nullptr);
            }
            else
            {
                ::WriteFile(Handle, Text, Length, &Written, nullptr);
            }
        }
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }

    static FORCEINLINE void MemoryBarrier()
    {
    #if PLATFORM_ARCHITECTURE_X86_64
        _mm_sfence();
    #endif
    }

    static FORCEINLINE int32 GetLastErrorString(String& OutErrorString)
    {
        const int32 LastError = ::GetLastError();
        const int32 Flags     = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        LPSTR MessageBuffer = nullptr;

        const uint32 MessageLength = FormatMessageA(Flags, nullptr, LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&MessageBuffer, 0, nullptr);
        OutErrorString.Clear();
        OutErrorString.Append(MessageBuffer, MessageLength);
        LocalFree(MessageBuffer);
        return LastError;
    }
};
