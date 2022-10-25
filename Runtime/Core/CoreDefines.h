#pragma once

#ifdef _MSC_VER
    #ifndef PLATFORM_COMPILER_MSVC
        #define PLATFORM_COMPILER_MSVC (1)
    #endif
#endif

#ifdef __clang__
    #ifndef PLATFORM_COMPILER_CLANG
        #define PLATFORM_COMPILER_CLANG (1)
    #endif
#endif

#ifdef __GNUC__
    #ifndef PLATFORM_COMPILER_GCC
        #define PLATFORM_COMPILER_GCC (1)
    #endif
#endif


#if (!PLATFORM_COMPILER_MSVC) || (!PLATFORM_COMPILER_CLANG) || (!PLATFORM_COMPILER_GCC)
    #ifndef PLATFORM_COMPILER_UNDEFINED
        #define PLATFORM_COMPILER_UNDEFINED
    #endif
#endif


#if (!PLATFORM_WINDOWS) && (!PLATFORM_MACOS)
    #error No platform defined
#endif

// TODO: Move asserts to separate module/header
#if !defined(PRODUCTION_BUILD)
    #ifndef ENABLE_ASSERTS 
        #define ENABLE_ASSERTS (1)
    #endif
#endif

#ifndef CHECK
    #ifdef NDEBUG
        // We should redefine this later
        #define REDEFINE_NDEBUG (1)
        // But un-define now so we can have asserts in release
        #undef NDEBUG
    #endif

    #include <cassert>
    
    #if ENABLE_ASSERTS
        #define CHECK(Condition) assert(Condition)
    #else
        #define CHECK(Condition) (void)(Condition)
    #endif

    #ifdef REDEFINE_NDEBUG
        #define NDEBUG (1)
    #endif
#endif


#ifndef SAFE_DELETE
    #define SAFE_DELETE(OutObject)     \
        do                             \
        {                              \
            if ((OutObject))           \
            {                          \
                delete (OutObject);    \
                (OutObject) = nullptr; \
            }                          \
        } while (false)
#endif

#ifndef SAFE_RELEASE
    #define SAFE_RELEASE(OutObject)     \
        do                              \
        {                               \
            if ((OutObject))            \
            {                           \
                (OutObject)->Release(); \
                (OutObject) = nullptr;  \
            }                           \
        } while (false)
#endif

#ifndef SAFE_ADD_REF
    #define SAFE_ADD_REF(OutObject)    \
        do                             \
        {                              \
            if ((OutObject))           \
            {                          \
                (OutObject)->AddRef(); \
            }                          \
        } while (false);
#endif


#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(Array) (sizeof(Array) / sizeof(Array[0]))
#endif


#define BIT(Bit)  (1 << Bit)
#define FLAG(Bit) BIT(Bit)


#ifndef UNREFERENCED_VARIABLE
    #define UNREFERENCED_VARIABLE(Variable) (void)(Variable)
#endif

/**
 * String preprocessor handling. There are two versions of STRING_CONCAT, this is so that
 * you can use __LINE__, __FILE__ etc. within the macro, therefore always use STRING_CONCAT
 */

#ifndef _STRING_CONCAT
    #define _STRING_CONCAT(x, y) x##y
#endif

#ifndef STRING_CONCAT
    #define STRING_CONCAT(x, y) _STRING_CONCAT(x, y)
#endif


#ifndef MULTILINE_STRING
    #define MULTILINE_STRING(...) #__VA_ARGS__
#endif


#define ENABLE_CONSTEXPR (1)

#if ENABLE_CONSTEXPR
    #define CONSTEXPR constexpr
#else
    #define CONSTEXPR
#endif


#define ENABLE_NODISCARD (1)

#if ENABLE_NODISCARD
    #define NODISCARD [[nodiscard]]
#else
    #define NODISCARD
#endif


#define ENABLE_MAYBE_UNUSED (1)

#if ENABLE_MAYBE_UNUSED
    #define MAYBE_UNUSED [[maybe_unused]]
#else
    #define MAYBE_UNUSED
#endif


#define STANDARD_ALIGNMENT (__STDCPP_DEFAULT_NEW_ALIGNMENT__)


#if PLATFORM_COMPILER_MSVC
    #include "Core/CompilerSpecific/CompilerMSVC.h"
#elif PLATFORM_COMPILER_CLANG
    #include "Core/CompilerSpecific/CompilerClang.h" 
#elif PLATFORM_COMPILER_GCC 
    #include "Core/CompilerSpecific/CompilerGCC.h"
#elif PLATFORM_COMPILER_UNDEFINED 
    #include "Core/CompilerSpecific/CompilerDefault.h"
    #error "Unknown Compiler"
#endif
