#pragma once
#include "Core/Core.h"
#include "Core/Generic/GenericThreadMisc.h"

#if PLATFORM_ARCHITECTURE_X86_X64
    #include <immintrin.h>
#endif

struct CORE_API FWindowsThreadMisc final : public FGenericThreadMisc
{
    static FGenericEvent*  CreateEvent(bool bManualReset);
    static FGenericThread* CreateThread(FThreadInterface* InRunnable, bool bSuspended = true);

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