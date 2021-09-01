#pragma once

/*
* MSVC compiler specific macros etc.
* For now this file should only be included into CoreDefines.h
*/

#if COMPILER_MSVC

/* Architecture */
#if defined(_M_IX86) || defined(_M_X64)
#ifndef ARCH_X86_X64
#define ARCH_X86_X64 (1)
#endif
#endif // ARCH_X86_X64

/* Use SSE intrinsics if we can*/
#if ARCH_X86_X64
#ifndef ENABLE_SEE_INTRIN
#define ENABLE_SEE_INTRIN (1)
#endif
#endif

/* Forceinline */
#ifndef FORCEINLINE
#ifndef DEBUG_BUILD
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif // ifdef DEBUG_BUILD
#endif // ifndef FORCEINLINE

/* Align */
#ifndef ALIGN
#define ALIGN(Alignment) __declspec(align((Alignment)))
#endif

/* No inlining at all */
#ifndef NOINLINE
#define NOINLINE __declspec(noinline)
#endif

/* Vectorcall */
#ifndef VECTORCALL
#define VECTORCALL __vectorcall
#endif

/* Function signature as a const char* string */
#ifndef FUNCTION_SIGNATURE
#define FUNCTION_SIGNATURE __FUNCTION__
#endif

/* Disable some warnings */
#pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable : 4324) // structure was padded due to alignment specifier

/* Declare warnings as errors */
#pragma warning(error : 4099) // wrong forward declaration
#pragma warning(error : 4150) // cannot call destructor on incomplete type
#pragma warning(error : 4239) // setting references to rvalues
#pragma warning(error : 4456) // variable hides a already existing variable
#pragma warning(error : 4458) // variable hides class member
#pragma warning(error : 4715) // not all paths return a value
#pragma warning(error : 4840) // using string in variadic template (When it should be const char)

#else
#error "MSVC Compiler-file included in non MSVC- compiler"
#endif