#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// MSVC compiler specific macros etc.
// 
// For now this file should only be included into CoreDefines.h

#if PLATFORM_COMPILER_MSVC

#ifndef PLATFORM_ARCHITECTURE_X86_X64
    #if defined(_M_IX86) || defined(_M_X64)
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
        #define FORCEINLINE __forceinline
    #else
        #define FORCEINLINE inline
    #endif
#endif


#ifndef ALIGN_AS
    #define ALIGN_AS(Alignment) __declspec(align((Alignment)))
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
    #define FUNCTION_SIGNATURE __FUNCTION__
#endif


#ifndef MODULE_EXPORT
    #define MODULE_EXPORT __declspec(dllexport)
#endif

#ifndef MODULE_IMPORT
    #define MODULE_IMPORT __declspec(dllimport)
#endif


#ifndef DEBUG_BREAK
    #define DEBUG_BREAK __debugbreak
#endif

// Define the rest of the defines to a default value
#include "CompilerDefault.h"

// Disable some warnings
#pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable : 4324) // structure was padded due to alignment specifier

// TODO: Investigate if this can be removed (4275, 4251)
#pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
#pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'

// Declare warnings as errors
#pragma warning(error : 4099) // wrong forward declaration
#pragma warning(error : 4150) // cannot call destructor on incomplete type
#pragma warning(error : 4239) // setting references to rvalues
#pragma warning(error : 4456) // variable hides a already existing variable
#pragma warning(error : 4458) // variable hides class member
#pragma warning(error : 4554) // check operator precedence for possible error; use parentheses to clarify precedence
#pragma warning(error : 4715) // not all paths return a value
#pragma warning(error : 4840) // using string in variadic template (When it should be const CHAR)

#else
   #error "MSVC Compiler-file included in non MSVC- compiler"
#endif
