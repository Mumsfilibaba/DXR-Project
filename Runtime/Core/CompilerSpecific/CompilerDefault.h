#pragma once

/*
* Defaults for compiler specific
* For now this file should only be included into CoreDefines.h
*/

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

/* Vector-call */
#ifndef VECTORCALL
#define VECTORCALL
#endif

/* Restrict */
#ifndef restrict_ptr
#define restrict_ptr
#endif

/* Function signature as a const char* string */
#ifndef FUNCTION_SIGNATURE
#define FUNCTION_SIGNATURE "NO COMPILER DEFINED, DEFINE TO GET THE FUNCTION_SIGNATURE"
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
#define PauseInstruction() (void)0
#endif
