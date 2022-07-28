#pragma once
#include "Core/Core.h"
#include "Core/Threading/Generic/GenericThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThreadMisc

struct FWindowsThreadMisc 
    : public FGenericThreadMisc
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericThreadMisc Interface

    static FGenericThread* CreateThread(const TFunction<void()>& InFunction);
    static FGenericThread* CreateNamedThread(const TFunction<void()>& InFunction, const FString& InString);

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

    static FORCEINLINE void Sleep(FTimestamp Time)
    {
        DWORD Milliseconds = static_cast<DWORD>(Time.AsMilliSeconds());
        ::Sleep(Milliseconds);
    }
};