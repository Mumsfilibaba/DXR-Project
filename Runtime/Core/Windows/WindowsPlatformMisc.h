#pragma once
#include "Windows.h"

#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Generic/GenericPlatformMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsPlatformMisc

struct FWindowsPlatformMisc final 
    : public FGenericPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const FString& Message)
    {
        OutputDebugStringA(Message.GetCString());
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

        const uint32 Flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        LPSTR  MessageBuffer = nullptr;
        uint32 MessageLength = FormatMessageA(Flags, nullptr, LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&MessageBuffer, 0, nullptr);

        OutErrorString.Clear();
        OutErrorString.Append(MessageBuffer, MessageLength);

        LocalFree(MessageBuffer);
        return LastError;
    }
};