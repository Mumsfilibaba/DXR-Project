#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Clang compiler-specific macros etc.
// This file should only be included into CoreDefines.h

#if PLATFORM_COMPILER_CLANG

// Use SSE intrinsics if we can
#if PLATFORM_ARCHITECTURE_X86_X64
    #ifndef ENABLE_SSE_INTRIN
        #define ENABLE_SSE_INTRIN (1)
    #endif
#endif

// Macro Definitions
#ifndef FORCEINLINE
    #ifndef DEBUG_BUILD
        #define FORCEINLINE inline __attribute__((always_inline))
    #else
        #define FORCEINLINE inline
    #endif
#endif

#ifndef ALIGN_AS
    #define ALIGN_AS(Alignment) __attribute__((aligned(Alignment)))
#endif

#ifndef NOINLINE
    #define NOINLINE __attribute__((noinline))
#endif

#ifndef VECTORCALL
    #if PLATFORM_WINDOWS && __has_attribute(vectorcall)
        #define VECTORCALL __vectorcall
    #else
        #define VECTORCALL
    #endif
#endif

#ifndef RESTRICT
    #define RESTRICT __restrict__
#endif

// Function signature as a const CHAR* string
#ifndef FUNCTION_SIGNATURE
    #define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif

#ifndef MODULE_EXPORT
    #if PLATFORM_WINDOWS
        #define MODULE_EXPORT __attribute__((dllexport))
    #else
        #define MODULE_EXPORT __attribute__((visibility("default")))
    #endif
#endif

#ifndef MODULE_IMPORT
    #if PLATFORM_WINDOWS
        #define MODULE_IMPORT __attribute__((dllimport))
    #else
        #define MODULE_IMPORT
    #endif
#endif

#ifndef DEBUG_BREAK
    #if !defined(PRODUCTION_BUILD)
        #define DEBUG_BREAK() __builtin_trap()
    #else
        #define DEBUG_BREAK() ((void)0)
    #endif
#endif

// Warning Control Macros

// Disable unreferenced variable warning
#if !defined(DISABLE_UNREFERENCED_VARIABLE_WARNING)
    #define DISABLE_UNREFERENCED_VARIABLE_WARNING \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wunused-parameter\"")
    #define ENABLE_UNREFERENCED_VARIABLE_WARNING \
        _Pragma("clang diagnostic pop")
#endif

// Disable unreachable code warning
#if !defined(DISABLE_UNREACHABLE_CODE_WARNING)
    #define DISABLE_UNREACHABLE_CODE_WARNING \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wunreachable-code\"")
    #define ENABLE_UNREACHABLE_CODE_WARNING \
        _Pragma("clang diagnostic pop")
#endif

// Disable hides previous local declaration warning
#if !defined(DISABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING)
    #define DISABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wshadow\"")
    #define ENABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING \
        _Pragma("clang diagnostic pop")
#endif

// Include default definitions
#include "CoreDefinesDefault.h"

#else
    #error "Clang Compiler-file included in non-Clang compiler"
#endif
