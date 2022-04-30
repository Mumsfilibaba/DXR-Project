#pragma once
#include "GenericThread.h"

#include "Core/Time/Timestamp.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericThreadMisc

class CGenericThreadMisc
{
public:

    static FORCEINLINE bool Initialize() { return true; }

    static CGenericThread* CreateThread(const TFunction<void()>& InFunction);

    static CGenericThread* CreateNamedThread(const TFunction<void()>& InFunction, const String& InName);

    static FORCEINLINE void Release() { }

    static FORCEINLINE uint32 GetNumProcessors() { return 1; }

    static FORCEINLINE void* GetThreadHandle() { return nullptr; }

    static FORCEINLINE void Sleep(CTimestamp Time) { }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
