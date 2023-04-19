#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GCC compiler specific macros etc.
// 
// For now this file should only be included into CoreDefines.h

#if PLATFORM_COMPILER_GCC

#ifndef PLATFORM_ARCHITECTURE_X86_X64
    #if defined(__x86_64__) || defined(__i386__)
        #define PLATFORM_ARCHITECTURE_X86_X64 (1)
    #else
        #define PLATFORM_ARCHITECTURE_X86_X64 (0)
    #endif
#endif

#if PLATFORM_ARCHITECTURE_X86_X64
    #ifndef ENABLE_SEE_INTRIN
        #define ENABLE_SEE_INTRIN (1)
    #endif
#endif


#ifndef FORCEINLINE
    #ifndef DEBUG_BUILD
        #define FORCEINLINE __attribute__((always_inline)) inline
    #else
        #define FORCEINLINE inline
    #endif
#endif


#ifndef ALIGN_AS
    #define ALIGN_AS(Alignment) __attribute__((aligned(Alignment)))
#endif


#ifndef NOINLINE
    #define NOINLINE __attribute__ ((noinline))
#endif


#ifndef VECTORCALL
    #define VECTORCALL // Does not seem to be supported
#endif


#ifndef RESTRICT
    #define RESTRICT __restrict
#endif


#ifndef FUNCTION_SIGNATURE
    #define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif


#ifndef MODULE_EXPORT
    #define MODULE_EXPORT __attribute__((visibility("default")))
#endif

#ifndef MODULE_IMPORT
    #define MODULE_IMPORT __attribute__((visibility("default")))
#endif


#ifndef DEBUG_BREAK
    #if !PRODUCTION_BUILD
        #define DEBUG_BREAK __builtin_trap
    #else
        #define DEBUG_BREAK
    #endif
#endif

// Define the rest of the defines to a default value
#include "CoreDefinesDefault.h"

#else
    #error "GCC Compiler-file included in non GCC- compiler"
#endif
