#pragma once

/*
* Clang compiler specific macros etc.
* For now this file should only be included into CoreDefines.h
*/

#if COMPILER_CLANG

/* Architecture */
#if defined(__x86_64__) || defined(__i386__)
#ifndef ARCHITECTURE_X86_X64
#define ARCHITECTURE_X86_X64 (1)
#endif
#endif // ARCHITECTURE_X86_X64

/* Use SSE intrinsics if we can*/
#if ARCHITECTURE_X86_X64
#ifndef ENABLE_SEE_INTRIN
#define ENABLE_SEE_INTRIN (1)
#endif
#endif

/* Forceinline */
#ifndef FORCEINLINE
#ifndef DEBUG_BUILD
#define FORCEINLINE __attribute__((always_inline)) inline
#else
#define FORCEINLINE inline
#endif // ifdef DEBUG_BUILD
#endif // ifndef FORCEINLINE

/* Align */
#ifndef ALIGN_AS
#define ALIGN_AS(Alignment) __attribute__((aligned(Alignment)))
#endif

/* No inlining at all */
#ifndef NOINLINE
#define NOINLINE __attribute__ ((noinline))
#endif

/* Vectorcall */
#ifndef VECTORCALL
#define VECTORCALL __vectorcall
#endif

/* Restric a pointer */
#ifndef restrict_ptr
#define restrict_ptr __restrict
#endif

/* Function signature as a const char* string */
#ifndef FUNCTION_SIGNATURE
#define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif

/* Dynamic Lib Export and import */
#ifndef MODULE_EXPORT
#define MODULE_EXPORT
#endif

#ifndef MODULE_IMPORT
#define MODULE_IMPORT
#endif

/* Pause the thread */
#ifndef PauseInstruction
#define PauseInstruction __builtin_ia32_pause
#endif

// Define the rest of the defines to a default value
#include "CompilerDefault.h"

#else
#error "Clang Compiler-file included in non Clang- compiler"
#endif
