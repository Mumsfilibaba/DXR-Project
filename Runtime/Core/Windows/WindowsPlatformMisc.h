#pragma once
#include "Windows.h"
#include "Core/Core.h"
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

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }

    static FORCEINLINE void MemoryBarrier()
    {
        _mm_sfence();
    }

    static FORCEINLINE int32 GetLastErrorString(FString& OutErrorString)
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