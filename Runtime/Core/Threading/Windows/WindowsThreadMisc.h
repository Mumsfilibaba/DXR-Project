#pragma once
#include "Core/Core.h"
#include "Core/Threading/Generic/GenericThreadMisc.h"

#if PLATFORM_ARCHITECTURE_X86_X64
    #include <immintrin.h>
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThreadMisc

struct FWindowsThreadMisc 
    : public FGenericThreadMisc
{
    static FGenericThread* CreateThread(const FThreadFunction& InFunction);
    static FGenericThread* CreateNamedThread(const FThreadFunction& InFunction, const FString& InString);
    static FGenericEvent*  CreateEvent(bool bManualReset);

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

    static FORCEINLINE void Sleep(FTimespan Time)
    {
        DWORD Milliseconds = static_cast<DWORD>(Time.AsMilliseconds());
        ::Sleep(Milliseconds);
    }

    static FORCEINLINE void Pause() 
    {
#if PLATFORM_ARCHITECTURE_X86_X64
        _mm_pause();
#endif
    }
};