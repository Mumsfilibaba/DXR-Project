#pragma once

/* Detect compiler */
#ifdef _MSC_VER
#ifndef COMPILER_MSVC
#define COMPILER_MSVC (1)
#endif
#endif

/* Undefined compiler */
#if !defined(COMPILER_MSVC)
#ifndef COMPILER_UNDEFINED
#define COMPILER_UNDEFINED
#endif
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

inline bool HasFlag( uint32 Mask, uint32 Flag )
{
    return Mask & Flag;
}

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
#elif COMPILER_UNDEFINED 
#include "Compiler/CompilerDefault.h"
#endif
