#pragma once
#include "Engine/EngineGlobals.h"

#include "Memory/New.h"

#include <string>

#include <DirectXMath.h>
using namespace DirectX;

// Assert
#ifdef DEBUG_BUILD
    #define ENABLE_ASSERTS 1
#endif

#ifndef Assert
#if ENABLE_ASSERTS
    #define Assert(Condition) assert(Condition)
#else
    #define Assert(Condition) (void)
#endif
#endif

// Macro for deleting objects safley
#define SafeDelete(OutObject) \
    if ((OutObject)) \
    { \
        delete (OutObject); \
        (OutObject) = nullptr; \
    }

#define SafeRelease(OutObject) \
    if ((OutObject)) \
    { \
        (OutObject)->Release(); \
        (OutObject) = nullptr; \
    }

#define SafeAddRef(OutObject) \
    if ((OutObject)) \
    { \
        (OutObject)->AddRef(); \
    }

// Helper Macros
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

//Forceinline
#ifndef FORCEINLINE

#ifndef DEBUG_BUILD
#ifdef COMPILER_VISUAL_STUDIO
    #define FORCEINLINE __forceinline
#else
    #define FORCEINLINE __attribute__((always_inline)) inline
#endif // ifdef COMPILER_VISUAL_STUDIO
#else
    #define FORCEINLINE inline
#endif // ifdef DEBUG_BUILD

#endif // ifndef FORCEINLINE

// Bit-Mask helpers
#define BIT(Bit)  (1 << Bit)
#define FLAG(Bit) BIT(Bit)

inline Bool HasFlag(UInt32 Mask, UInt32 Flag)
{
    return Mask & Flag;
}

// Unused params
#ifndef UNREFERENCED_VARIABLE
    #define UNREFERENCED_VARIABLE(Variable) (void)(Variable)
#endif

/*
* String preprocessor handling
*   There are two versions of PREPROCESS_CONCAT, this is so that you can use __LINE__, __FILE__ etc. within the macro,
*   therefore always use PREPROCESS_CONCAT
*/

#define _PREPROCESS_CONCAT(x, y) x##y
#define PREPROCESS_CONCAT(x, y) _PREPROCESS_CONCAT(x, y)

// Makes multiline strings
#define MULTILINE_STRING(...) #__VA_ARGS__

// Function signature as a const Char* string
#ifdef COMPILER_VISUAL_STUDIO
    #define __FUNCTION_SIG__ __FUNCTION__
#else
    #define __FUNCTION_SIG__ __PRETTY_FUNCTION__
#endif

// Disable some warnings
#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(disable : 4201) // nonstandard extension used: nameless struct/union
    #pragma warning(disable : 4324) // structure was padded due to alignment specifier
#endif

// Declare warnings as errors
#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(error : 4099) // wrong forward declaration
    #pragma warning(error : 4150) // cannot call destructor on incomplete type
    #pragma warning(error : 4239) // setting references to rvalues
    #pragma warning(error : 4456) // variable hides a already existing variable
    #pragma warning(error : 4458) // variable hides class member
    #pragma warning(error : 4715) // not all paths return a value
    #pragma warning(error : 4840) // using string in variadic template (When it should be const Char)
#endif