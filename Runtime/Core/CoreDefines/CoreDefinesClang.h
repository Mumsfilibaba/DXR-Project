#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Clang compiler specific macros etc.
// 
// For now this file should only be included into CoreDefines.h

#if PLATFORM_COMPILER_CLANG

#ifndef PLATFORM_ARCHITECTURE_X86_X64
    #if defined(__x86_64__) || defined(__i386__)
        #define PLATFORM_ARCHITECTURE_X86_X64 (1)
    #else
        #define PLATFORM_ARCHITECTURE_X86_X64 (0)
    #endif
#endif

// Use SSE intrinsics if we can
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
    #if PLATFORM_ARCHITECTURE_X86_X64
        #define VECTORCALL __vectorcall
    #else
        #define VECTORCALL
    #endif
#endif


#ifndef RESTRICT
    #define RESTRICT __restrict
#endif


// Function signature as a const CHAR* string
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
    #define DEBUG_BREAK __builtin_trap
#endif

// Define the rest of the defines to a default value
#include "CoreDefinesDefault.h"

// Disable unreferenced variable warning
#if !defined(DISABLE_UNREFERENCED_VARIABLE_WARNING)
    #define DISABLE_UNREFERENCED_VARIABLE_WARNING                  \
        _Pragma("clang diagnostic push")                           \
        _Pragma("clang diagnostic ignored \"-Wunused-parameter\"") 
    #define ENABLE_UNREFERENCED_VARIABLE_WARNING \
        _Pragma("clang diagnostic pop")
#endif

// TODO: Finish up
// Disable unreachable code warning
#if !defined(DISABLE_UNREACHABLE_CODE_WARNING)
    #define DISABLE_UNREACHABLE_CODE_WARNING
    #define ENABLE_UNREACHABLE_CODE_WARNING
#endif

#else
    #error "Clang Compiler-file included in non Clang- compiler"
#endif