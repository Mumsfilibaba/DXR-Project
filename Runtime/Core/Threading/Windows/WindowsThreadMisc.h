#pragma once
#include "Core/Core.h"
#include "Core/Threading/Generic/GenericThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsThreadMisc

class CWindowsThreadMisc : public CGenericThreadMisc
{
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericThreadMisc Interface

    static CGenericThread* CreateThread(const TFunction<void()>& InFunction);

    static CGenericThread* CreateNamedThread(const TFunction<void()>& InFunction, const FString& InString);

    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        FMemory::Memzero(&SystemInfo);

        GetSystemInfo(&SystemInfo);

        return static_cast<uint32>(SystemInfo.dwNumberOfProcessors);
    }

    static FORCEINLINE void* GetThreadHandle()
    {
        DWORD CurrentHandle = GetCurrentThreadId();
        return reinterpret_cast<void*>(static_cast<uintptr_t>(CurrentHandle));
    }

    static FORCEINLINE void Sleep(CTimestamp Time)
    {
        DWORD Milliseconds = static_cast<DWORD>(Time.AsMilliSeconds());
        ::Sleep(Milliseconds);
    }
};