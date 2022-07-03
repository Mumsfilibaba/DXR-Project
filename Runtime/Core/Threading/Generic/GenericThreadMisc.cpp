#include "GenericThreadMisc.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericThreadMisc

FGenericThread* FGenericThreadMisc::CreateThread(const TFunction<void()>& InFunction)
{
    return dbg_new FGenericThread(InFunction);
}

FGenericThread* FGenericThreadMisc::CreateNamedThread(const TFunction<void()>& InFunction, const FString& InName)
{
    return dbg_new FGenericThread(InFunction);
}

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
