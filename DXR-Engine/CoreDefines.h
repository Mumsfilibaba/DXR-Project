#pragma once

/* Detect compiler */
#ifdef _MSC_VER
#ifndef COMPILER_MSVC
#define COMPILER_MSVC (1)
#endif
#endif

#ifdef __clang__
#ifndef COMPILER_CLANG
#define COMPILER_CLANG (1)
#endif
#endif

#ifdef __GNUC__
#ifndef COMPILER_GCC
#define COMPILER_GCC (1)
#endif
#endif

/* Undefined compiler */
#if !COMPILER_MSVC || !COMPILER_CLANG || !COMPILER_GCC
#ifndef COMPILER_UNDEFINED
#define COMPILER_UNDEFINED
#endif
#endif

/* Check that a platform is defined */
#if (!defined(PLATFORM_WINDOWS)) && (!defined(PLATFORM_MACOS))
#error No platform defined
#endif

// TODO: Move asserts to seperate module/header

/* Asserts */
#ifdef DEBUG_BUILD
#ifndef ENABLE_ASSERTS 
#define ENABLE_ASSERTS 1
#endif
#endif

#ifndef Assert
#include <cassert>

#if ENABLE_ASSERTS
#define Assert(Condition) assert(Condition)
#else
#define Assert(Condition) (void)(0)
#endif
#endif

/* Macro for deleting objects safley */

#ifndef SafeDelete
#define SafeDelete(OutObject) \
    if ((OutObject)) \
    { \
        delete (OutObject); \
        (OutObject) = nullptr; \
    }
#endif

#ifndef SafeRelease
#define SafeRelease(OutObject) \
    if ((OutObject)) \
    { \
        (OutObject)->Release(); \
        (OutObject) = nullptr; \
    }
#endif

#ifndef SafeAddRef
#define SafeAddRef(OutObject) \
    if ((OutObject)) \
    { \
        (OutObject)->AddRef(); \
    }
#endif

/* Helper Macros */

#ifndef ArrayCount
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))
#endif

/* Bit-Mask helpers */

#define BIT(Bit)  (1 << Bit)
#define FLAG(Bit) BIT(Bit)

/* Unused params */

#ifndef UNREFERENCED_VARIABLE
#define UNREFERENCED_VARIABLE(Variable) (void)(Variable)
#endif

/*
* String preprocessor handling. There are two versions of PREPROCESS_CONCAT, this is so that
* you can use __LINE__, __FILE__ etc. within the macro, therefore always use PREPROCESS_CONCAT
*/

#ifndef _PREPROCESS_CONCAT
#define _PREPROCESS_CONCAT(x, y) x##y
#endif

#ifndef PREPROCESS_CONCAT
#define PREPROCESS_CONCAT(x, y) _PREPROCESS_CONCAT(x, y)
#endif

/* Makes multiline strings */
#ifndef MULTILINE_STRING
#define MULTILINE_STRING(...) #__VA_ARGS__
#endif

/* Compiler Specific */
#if COMPILER_MSVC
#include "Compiler/CompilerMSVC.h"
#elif COMPILER_CLANG
#include "Compiler/CompilerClang.h" 
#elif COMPILER_GCC 
#include "Compiler/CompilerGCC.h"
#elif COMPILER_UNDEFINED 
#include "Compiler/CompilerDefault.h"
#error "Unknown Compiler"
#endif
