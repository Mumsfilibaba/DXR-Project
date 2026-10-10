#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

// -------------------------------------------------------------------------------------------------
// Assertion Control
// -------------------------------------------------------------------------------------------------

#ifndef ENABLE_ASSERTS
    #if !defined(RELEASE_BUILD)
        #define ENABLE_ASSERTS (1)
    #else
        #define ENABLE_ASSERTS (0)
    #endif
#endif

/** @brief What a failing call site should do once the handler has reported the assertion */
enum class EAssertAction : uint8
{
    Continue = 0,
    Break    = 1,
};

struct CORE_API Assert
{
    static EAssertAction OnFailed(const CHAR_T* Expression, const CHAR_T* Filename, int32 Line, const CHAR_T* Format, ...);
};

#if ENABLE_ASSERTS
    #define CHECK(Condition) \
        do \
        { \
            if (!(Condition)) \
            { \
                if (Assert::OnFailed(TEXT(#Condition), TEXT(__FILE__), __LINE__, nullptr) == EAssertAction::Break) \
                { \
                    DEBUG_BREAK(); \
                } \
            } \
        } while (false)

    #define CHECKF(Condition, Format, ...) \
        do \
        { \
            if (!(Condition)) \
            { \
                if (Assert::OnFailed(TEXT(#Condition), TEXT(__FILE__), __LINE__, TEXT("" Format) __VA_OPT__(,) __VA_ARGS__) == EAssertAction::Break) \
                { \
                    DEBUG_BREAK(); \
                } \
            } \
        } while (false)

    #define VERIFY(Condition) CHECK(Condition)
#else
    #define CHECK(Condition)               ((void)sizeof(!(Condition)))
    #define CHECKF(Condition, Format, ...) ((void)sizeof(!(Condition)))
    #define VERIFY(Condition)              ((void)(Condition))
#endif
