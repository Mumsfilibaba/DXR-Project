#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// MSVC compiler-specific macros etc.
// This file should only be included into CoreDefines.h

#if PLATFORM_COMPILER_MSVC

#if PLATFORM_ARCHITECTURE_X86_X64
    #ifndef ENABLE_SSE_INTRIN
        #define ENABLE_SSE_INTRIN (1)
    #endif
#endif

// Macro Definitions
#ifndef FORCEINLINE
    #ifndef DEBUG_BUILD
        #define FORCEINLINE __forceinline
    #else
        #define FORCEINLINE inline
    #endif
#endif

#ifndef ALIGN_AS
    #define ALIGN_AS(Alignment) __declspec(align(Alignment))
#endif

#ifndef NOINLINE
    #define NOINLINE __declspec(noinline)
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

#ifndef FUNCTION_SIGNATURE
    #define FUNCTION_SIGNATURE __FUNCSIG__
#endif

#ifndef MODULE_EXPORT
    #define MODULE_EXPORT __declspec(dllexport)
#endif

#ifndef MODULE_IMPORT
    #define MODULE_IMPORT __declspec(dllimport)
#endif

#ifndef DEBUG_BREAK
    #if !defined(PRODUCTION_BUILD)
        #define DEBUG_BREAK() __debugbreak()
    #else
        #define DEBUG_BREAK() ((void)0)
    #endif
#endif

// Warning Control Macros

// Disable unreferenced variable warning
#if !defined(DISABLE_UNREFERENCED_VARIABLE_WARNING)
    #define DISABLE_UNREFERENCED_VARIABLE_WARNING \
        __pragma(warning(push)) \
        __pragma(warning(disable : 4100))
    #define ENABLE_UNREFERENCED_VARIABLE_WARNING \
        __pragma(warning(pop))
#endif

// Disable unreachable code warning
#if !defined(DISABLE_UNREACHABLE_CODE_WARNING)
    #define DISABLE_UNREACHABLE_CODE_WARNING \
        __pragma(warning(push)) \
        __pragma(warning(disable : 4702))
    #define ENABLE_UNREACHABLE_CODE_WARNING \
        __pragma(warning(pop))
#endif

// Disable hides previous local declaration warning
#if !defined(DISABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING)
    #define DISABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING \
        __pragma(warning(push)) \
        __pragma(warning(disable : 4456 4458))
    #define ENABLE_HIDES_PREVIOUS_LOCAL_DEFINITION_WARNING \
        __pragma(warning(pop))
#endif

// Include default definitions
#include "CoreDefinesDefault.h"

// Disable some warnings
#pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#pragma warning(disable : 5054) // operator '==': deprecated between enumerations of different types

// TODO: Investigate if this can be removed (4275, 4251)
#pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
#pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'

// Declare warnings as errors
#pragma warning(error : 4099) // wrong forward declaration
#pragma warning(error : 4150) // cannot call destructor on incomplete type
#pragma warning(error : 4239) // setting references to rvalues
#pragma warning(error : 4456) // variable hides an already existing variable
#pragma warning(error : 4458) // variable hides class member
#pragma warning(error : 4554) // check operator precedence for possible error; use parentheses to clarify precedence
#pragma warning(error : 4715) // not all paths return a value
#pragma warning(error : 4840) // using string in variadic template (When it should be const CHAR)

#else
    #error "MSVC Compiler-file included in non-MSVC compiler"
#endif
