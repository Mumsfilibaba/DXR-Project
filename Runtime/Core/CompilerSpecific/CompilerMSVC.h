#pragma once

/*
* MSVC compiler specific macros etc.
* For now this file should only be included into CoreDefines.h
*/

#if COMPILER_MSVC

// Architecture
#if defined(_M_IX86) || defined(_M_X64)
#ifndef ARCHITECTURE_X86_X64
#define ARCHITECTURE_X86_X64 (1)
#endif
#endif // ARCHITECTURE_X86_X64

// Use SSE intrinsics if we can
#if ARCHITECTURE_X86_X64
#ifndef ENABLE_SEE_INTRIN
#define ENABLE_SEE_INTRIN (1)
#endif
#endif

// Forceinline
#ifndef FORCEINLINE
#ifndef DEBUG_BUILD
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif // ifdef DEBUG_BUILD
#endif // ifndef FORCEINLINE

// Align
#ifndef ALIGN_AS
#define ALIGN_AS(Alignment) __declspec(align((Alignment)))
#endif

// No inlining at all
#ifndef NOINLINE
#define NOINLINE __declspec(noinline)
#endif

// Vector call
#ifndef VECTORCALL
#define VECTORCALL __vectorcall
#endif

// Restrict
#ifndef restrict_ptr
#define restrict_ptr __restrict
#endif

// Function signature as a const char* string
#ifndef FUNCTION_SIGNATURE
#define FUNCTION_SIGNATURE __FUNCTION__
#endif

// Dynamic Lib Export and import
#ifndef MODULE_EXPORT
#define MODULE_EXPORT __declspec(dllexport)
#endif

#ifndef MODULE_IMPORT
#define MODULE_IMPORT __declspec(dllimport)
#endif

// Pause the thread
#ifndef PauseInstruction
#include <immintrin.h>
#define PauseInstruction _mm_pause
#endif

// Define the rest of the defines to a default value
#include "CompilerDefault.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Disable some warnings

#pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
// TODO: Investigate if this can be removed
#pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
#pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'

/* Declare warnings as errors */
#pragma warning(error : 4099) // wrong forward declaration
#pragma warning(error : 4150) // cannot call destructor on incomplete type
#pragma warning(error : 4239) // setting references to rvalues
#pragma warning(error : 4456) // variable hides a already existing variable
#pragma warning(error : 4458) // variable hides class member
#pragma warning(error : 4554) // check operator precedence for possible error; use parentheses to clarify precedence
#pragma warning(error : 4715) // not all paths return a value
#pragma warning(error : 4840) // using string in variadic template (When it should be const char)

#else
#error "MSVC Compiler-file included in non MSVC- compiler"
#endif
