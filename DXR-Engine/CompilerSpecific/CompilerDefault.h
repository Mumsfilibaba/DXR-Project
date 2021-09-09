#pragma once

/*
* Defaults for compiler specific
* For now this file should only be included into CoreDefines.h
*/

#if COMPILER_UNDEFINED

/* Forceinline */
#ifndef FORCEINLINE
#define FORCEINLINE inline
#endif

/* Align */
#ifndef ALIGN
#define ALIGN
#endif

/* No inlining at all */
#ifndef NOINLINE
#define NOINLINE
#endif

/* Vectorcall */
#ifndef VECTORCALL
#define VECTORCALL
#endif

/* Restric */
#ifndef RESTRICT
#define RESTRICT
#endif

/* Function signature as a const char* string */
#ifndef FUNCTION_SIGNATURE
#define FUNCTION_SIGNATURE "NO COMPILER DEFINED, DEFINE TO GET THE FUNCTION_SIGNATURE"
#endif

#endif