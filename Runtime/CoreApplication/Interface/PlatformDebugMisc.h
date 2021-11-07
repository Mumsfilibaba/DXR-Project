#pragma once
#include "Core/Containers/String.h"

#include "CoreApplication/CoreApplicationModule.h"

#ifdef MessageBox
#undef MessageBox
#endif

#ifdef OutputDebugString
#undef OutputDebugString
#endif

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CPlatformDebugMisc
{
public:

    /* If the debugger is attached, a breakpoint will be set at this point of the code */
    static FORCEINLINE void DebugBreak() {}

    /* Outputs a debug string to the attached debugger */
    static FORCEINLINE void OutputDebugString( const CString& Message ) {}

    /* Checks weather or not the application is running inside a debugger */
    static FORCEINLINE bool IsDebuggerPresent() { return false; }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
