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
#if (!COMPILER_MSVC) || (!COMPILER_CLANG) || (!COMPILER_GCC)
#ifndef COMPILER_UNDEFINED
#define COMPILER_UNDEFINED
#endif
#endif

/* Check that a platform is defined */
#if (!PLATFORM_WINDOWS) && (!PLATFORM_MACOS)
#error No platform defined
#endif

// TODO: Move asserts to separate module/header

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

/* Macro for deleting objects safely */

#ifndef SafeDelete
#define SafeDelete(OutObject)      \
    do                             \
    {                              \
        if ((OutObject))           \
        {                          \
            delete (OutObject);    \
            (OutObject) = nullptr; \
        }                          \
    } while (false)
#endif

#ifndef SafeRelease
#define SafeRelease(OutObject)      \
    do                              \
    {                               \
        if ((OutObject))            \
        {                           \
            (OutObject)->Release(); \
            (OutObject) = nullptr;  \
        }                           \
    } while (false)
#endif

#ifndef SafeAddRef
#define SafeAddRef(OutObject)      \
    do                             \
    {                              \
        if ((OutObject))           \
        {                          \
            (OutObject)->AddRef(); \
        }                          \
    } while (false);
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
#include "Core/CompilerSpecific/CompilerMSVC.h"
#elif COMPILER_CLANG
#include "Core/CompilerSpecific/CompilerClang.h" 
#elif COMPILER_GCC 
#include "Core/CompilerSpecific/CompilerGCC.h"
#elif COMPILER_UNDEFINED 
#include "Core/CompilerSpecific/CompilerDefault.h"
#error "Unknown Compiler"
#endif
