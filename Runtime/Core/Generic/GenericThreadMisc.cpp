#include "GenericThreadMisc.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericThreadMisc

FGenericThread* FGenericThreadMisc::CreateThread(FThreadInterface* Runnable)
{
    return dbg_new FGenericThread(Runnable);
}

FGenericEvent* FGenericThreadMisc::CreateEvent(bool bManualReset)
{
    return dbg_new FGenericEvent();
}

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
