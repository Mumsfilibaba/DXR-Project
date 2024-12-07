#pragma once
#include <cassert>
#include <cstdint>
#include <cstddef> // For std::max_align_t and offsetof
#include <new>     // For __STDCPP_DEFAULT_NEW_ALIGNMENT__

// Define __has_cpp_attribute if not defined
#ifndef __has_cpp_attribute
    #define __has_cpp_attribute(x) 0
#endif

// Compiler Detection
#ifdef _MSC_VER
    #ifndef PLATFORM_COMPILER_MSVC
        #define PLATFORM_COMPILER_MSVC (1)
    #endif
#elif defined(__clang__)
    #ifndef PLATFORM_COMPILER_CLANG
        #define PLATFORM_COMPILER_CLANG (1)
    #endif
#elif defined(__GNUC__) && !defined(__clang__)
    #ifndef PLATFORM_COMPILER_GCC
        #define PLATFORM_COMPILER_GCC (1)
    #endif
#else
    #error "Unknown Compiler"
#endif

// Define the standard SSE macros since MSVC does not do this for us
#if PLATFORM_COMPILER_MSVC
    #if defined(_M_IX86_FP) && (_M_IX86_FP >= 1)
        #define __SSE__
    #endif
    #if defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        #define __SSE__
        #define __SSE2__
        #define __SSE3__
        #define __SSSE3__
        #define __SSE4_1__
        #define __SSE4_2__
    #endif
#endif

// Platform Detection
#if defined(_WIN32) || defined(_WIN64)
    #ifndef PLATFORM_WINDOWS
        #define PLATFORM_WINDOWS (1)
    #endif
#elif defined(__APPLE__) && defined(__MACH__)
    #ifndef PLATFORM_MACOS
        #define PLATFORM_MACOS (1)
    #endif
#else
    #error "Unsupported platform"
#endif

// Architecture Detection

// x86/x64 Architecture
#if defined(_M_X64) || defined(__amd64__) || defined(__x86_64__)
    #ifndef PLATFORM_ARCHITECTURE_X86_X64
        #define PLATFORM_ARCHITECTURE_X86_X64 (1)
    #endif
#else
    #define PLATFORM_ARCHITECTURE_X86_X64 (0)
#endif

// ARM64 Architecture
#if defined(__aarch64__) || defined(_M_ARM64)
    #ifndef PLATFORM_ARCHITECTURE_ARM64
        #define PLATFORM_ARCHITECTURE_ARM64 (1)
    #endif
#else
    #define PLATFORM_ARCHITECTURE_ARM64 (0)
#endif

// ARM32 Architecture
#if defined(__arm__) || defined(_M_ARM)
    #ifndef PLATFORM_ARCHITECTURE_ARM32
        #define PLATFORM_ARCHITECTURE_ARM32 (1)
    #endif
#else
    #define PLATFORM_ARCHITECTURE_ARM32 (0)
#endif

// Determine if 64-bit architecture
#if PLATFORM_ARCHITECTURE_X86_X64 || PLATFORM_ARCHITECTURE_ARM64
    #define PLATFORM_64BIT (1)
#else
    #define PLATFORM_64BIT (0)
#endif

// Check for SSE intrinsics support
#if PLATFORM_ARCHITECTURE_X86_X64
    // SSE
    #if defined(__SSE__)
        #ifndef PLATFORM_SUPPORT_SSE_INTRIN
            #define PLATFORM_SUPPORT_SSE_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_SSE_INTRIN (0)
    #endif

    // SSE2
    #if defined(__SSE2__)
        #ifndef PLATFORM_SUPPORT_SSE2_INTRIN
            #define PLATFORM_SUPPORT_SSE2_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_SSE2_INTRIN (0)
    #endif

    // SSE3
    #if defined(__SSE3__)
        #ifndef PLATFORM_SUPPORT_SSE3_INTRIN
            #define PLATFORM_SUPPORT_SSE3_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_SSE3_INTRIN (0)
    #endif

    // SSSE3
    #if defined(__SSSE3__)
        #ifndef PLATFORM_SUPPORT_SSSE3_INTRIN
            #define PLATFORM_SUPPORT_SSSE3_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_SSSE3_INTRIN (0)
    #endif

    // SSE4.1
    #if defined(__SSE4_1__)
        #ifndef PLATFORM_SUPPORT_SSE4_1_INTRIN
            #define PLATFORM_SUPPORT_SSE4_1_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_SSE4_1_INTRIN (0)
    #endif

    // SSE4.2
    #if defined(__SSE4_2__)
        #ifndef PLATFORM_SUPPORT_SSE4_2_INTRIN
            #define PLATFORM_SUPPORT_SSE4_2_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_SSE4_2_INTRIN (0)
    #endif

    // AVX
    #if defined(__AVX__)
        #ifndef PLATFORM_SUPPORT_AVX_INTRIN
            #define PLATFORM_SUPPORT_AVX_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_AVX_INTRIN (0)
    #endif

    // AVX2
    #if defined(__AVX2__)
        #ifndef PLATFORM_SUPPORT_AVX2_INTRIN
            #define PLATFORM_SUPPORT_AVX2_INTRIN (1)
        #endif
    #else
        #define PLATFORM_SUPPORT_AVX2_INTRIN (0)
    #endif
#endif

// Assertion Control
#ifndef ENABLE_ASSERTS
    #if !defined(PRODUCTION_BUILD)
        #define ENABLE_ASSERTS (1)
    #else
        #define ENABLE_ASSERTS (0)
    #endif
#endif

#if ENABLE_ASSERTS
    #define CHECK(Condition) assert(Condition)
#else
    #define CHECK(Condition) ((void)0)
#endif

// Utility Macros
#ifndef SAFE_DELETE
    #define SAFE_DELETE(OutObject) \
        do { \
            delete (OutObject); \
            (OutObject) = nullptr; \
        } while (false)
#endif

#ifndef SAFE_RELEASE
    #define SAFE_RELEASE(OutObject) \
        do { \
            if ((OutObject)) { \
                (OutObject)->Release(); \
                (OutObject) = nullptr; \
            } \
        } while (false)
#endif

#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))
#endif

#ifndef OFFSETOF
    #define OFFSETOF(Type, Member) offsetof(Type, Member)
#endif

#define BIT(Bit)  (1 << (Bit))
#define FLAG(Bit) BIT(Bit)

#ifndef UNREFERENCED_VARIABLE
    #define UNREFERENCED_VARIABLE(Variable) (void)(Variable)
#endif

// String Preprocessor Handling
#ifndef _STRING_CONCAT
    #define _STRING_CONCAT(x, y) x##y
#endif

#ifndef STRING_CONCAT
    #define STRING_CONCAT(x, y) _STRING_CONCAT(x, y)
#endif

#ifndef MULTILINE_STRING
    #define MULTILINE_STRING(...) #__VA_ARGS__
#endif

// Attributes
#if (__cplusplus >= 201703L) && __has_cpp_attribute(nodiscard)
    #define NODISCARD [[nodiscard]]
#else
    #define NODISCARD
#endif

#if (__cplusplus >= 201703L) && __has_cpp_attribute(maybe_unused)
    #define MAYBE_UNUSED [[maybe_unused]]
#else
    #define MAYBE_UNUSED
#endif

#if (__cplusplus >= 201402L) && __has_cpp_attribute(deprecated)
    #define DEPRECATED(Message) [[deprecated(Message)]]
#else
    #define DEPRECATED(Message)
#endif

// Standard Alignment
#if (__cplusplus >= 201703L) && defined(__cpp_aligned_new)
    #define STANDARD_ALIGNMENT (__STDCPP_DEFAULT_NEW_ALIGNMENT__)
#else
    #define STANDARD_ALIGNMENT (alignof(std::max_align_t))
#endif

// Include Compiler-Specific Definitions
#if PLATFORM_COMPILER_MSVC
    #include "Core/CoreDefines/CoreDefinesMSVC.h"
#elif PLATFORM_COMPILER_CLANG
    #include "Core/CoreDefines/CoreDefinesClang.h"
#elif PLATFORM_COMPILER_GCC
    #include "Core/CoreDefines/CoreDefinesGCC.h"
#else
    #include "Core/CoreDefines/CoreDefinesDefault.h"
#endif
