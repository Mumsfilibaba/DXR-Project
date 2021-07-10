#pragma once
#include <cassert>

//#define DISABLE_SIMD

// TODO: Remove from here
#define ASSERT(bCondition) (assert(bCondition))

/* Windows Specific */
#if defined(_WIN32)

#define ALIGN_16    __declspec(align(16))

#define FORCEINLINE __forceinline

#define NOINLINE    __declspec(noinline)

#define VECTORCALL  __vectorcall

// TODO: Check for x86/x64
#define X86_X64     (1)

#else

#define ALIGN_16

#define FORCEINLINE inline

#define NOINLINE

#define VECTORCALL

#define X86_X64     (0)

#endif