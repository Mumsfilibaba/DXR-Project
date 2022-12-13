#pragma once

#ifndef PLATFORM_ARCHITECTURE_X86_X64
    #define PLATFORM_ARCHITECTURE_X86_X64 (0)
#endif

#ifndef FORCEINLINE
    #define FORCEINLINE inline
#endif

#ifndef ALIGN_AS
    #define ALIGN_AS(Alignment) alignas(Alignment)
#endif

#ifndef NOINLINE
    #define NOINLINE
#endif

#ifndef VECTORCALL
    #define VECTORCALL
#endif

#ifndef RESTRICT
    #define RESTRICT
#endif

#ifndef FUNCTION_SIGNATURE
    #define FUNCTION_SIGNATURE "NO COMPILER DEFINED, DEFINE TO GET THE FUNCTION_SIGNATURE"
#endif

#ifndef MODULE_EXPORT
    #define MODULE_EXPORT
#endif

#ifndef MODULE_IMPORT
    #define MODULE_IMPORT
#endif

#ifndef DEBUG_BREAK
    #define DEBUG_BREAK() ((void)0)
#endif