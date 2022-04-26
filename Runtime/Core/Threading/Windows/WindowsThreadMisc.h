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

    static FORCEINLINE bool Initialize()
    {
        // This must be executed on the main-thread
        MainThreadHandle = GetCurrentThreadId();
        return true;
    }

    static FORCEINLINE uint32 GetNumProcessors()
    {
        SYSTEM_INFO SystemInfo;
        CMemory::Memzero(&SystemInfo);

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

    static FORCEINLINE bool IsMainThread()
    {
        return (MainThreadHandle == GetCurrentThreadId());
    }

private:
    static CORE_API DWORD MainThreadHandle;
};