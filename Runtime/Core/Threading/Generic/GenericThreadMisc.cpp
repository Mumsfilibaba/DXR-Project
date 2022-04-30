#include "GenericThreadMisc.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericThreadMisc

CGenericThread* CGenericThreadMisc::CreateThread(const TFunction<void()>& InFunction)
{
    return dbg_new CGenericThread(InFunction);
}

CGenericThread* CGenericThreadMisc::CreateNamedThread(const TFunction<void()>& InFunction, const String& InName)
{
    return dbg_new CGenericThread(InFunction);
}

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
