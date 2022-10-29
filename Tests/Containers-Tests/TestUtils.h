#pragma once
#include <iostream>
#include <cassert>

#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>

template<typename CHARTYPE>
FORCEINLINE void PrintString(const CHARTYPE* String);

template<>
FORCEINLINE void PrintString<CHAR>(const CHAR* String)
{
    std::cout << String;
}

template<>
FORCEINLINE void PrintString<WIDECHAR>(const WIDECHAR* String)
{
    std::wcout << String;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper macro for tests

#define MAKE_STRING(...) #__VA_ARGS__
#define MAKE_VAR(...)     __VA_ARGS__

#define TEST_CHECK(bCondition)                                                             \
    if (!(bCondition))                                                                \
    {                                                                                 \
        std::cout << "[TEST FAILED] Condition='" << MAKE_STRING(bCondition) << "'\n"; \
        assert(false);                                                                \
        return false;                                                                 \
    }                                                                                 \
    else                                                                              \
    {                                                                                 \
        std::cout << "[TEST SUCCEEDED]: '" << MAKE_STRING(bCondition) << "'\n";       \
    }

#define TEST_CHECK_ARRAY(Array, ...)                                                                                                  \
    if (Array.IsEmpty())                                                                                                         \
    {                                                                                                                            \
        std::cout << "[TEST FAILED] '" << MAKE_STRING(Array) "' is empty\n";                                                     \
        assert(false);                                                                                                           \
        return false;                                                                                                            \
    }                                                                                                                            \
    else                                                                                                                         \
    {                                                                                                                            \
        std::initializer_list InitList = MAKE_VAR(__VA_ARGS__);                                                                  \
                                                                                                                                 \
        int32 Index = 0;                                                                                                         \
        for (auto Element : InitList)                                                                                            \
        {                                                                                                                        \
            if ((Index >= static_cast<int32>(Array.GetSize())) || (Array[Index] != Element))                                     \
            {                                                                                                                    \
                std::cout << "[TEST FAILED] '" << MAKE_STRING(Array) << '[' << Index << ']' << " != " << Element[Index] << '\n'; \
                assert(false);                                                                                                   \
                return false;                                                                                                    \
            }                                                                                                                    \
                                                                                                                                 \
            Index++;                                                                                                             \
        }                                                                                                                        \
                                                                                                                                 \
        std::cout << "[TEST SUCCEEDED]: '" << MAKE_STRING(Array) << "' == " << MAKE_STRING(__VA_ARGS__) << '\n';                 \
    }

#define TEST_CHECK_STRING_N(String, Value, NumChars)                                                         \
    {                                                                                                   \
        const bool bResult = String.IsEmpty() ?                                                         \
            (TCString<decltype(String)::CharType>::Strlen(Value) == 0) :                                \
            (TCString<decltype(String)::CharType>::Strncmp(String.GetCString(), Value, NumChars) == 0); \
        if (!bResult)                                                                                   \
        {                                                                                               \
            std::cout << "[TEST FAILED] Condition=" << #String << "='";                                 \
            PrintString<decltype(String)::CharType>(String.GetCString());                               \
            std::cout << "'\n";                                                                         \
            std::cout << #String << "='";                                                               \
            PrintString<decltype(String)::CharType>(Value);                                             \
            std::cout << "'\n";                                                                         \
            assert(false);                                                                              \
            return false;                                                                               \
        }                                                                                               \
        else                                                                                            \
        {                                                                                               \
            std::cout << "[TEST SUCCEEDED]: " << #String << "='";                                       \
            PrintString<decltype(String)::CharType>(String.GetCString());                               \
            std::cout << "'\n";                                                                         \
        }                                                                                               \
    }

#define TEST_CHECK_STRING(String, Value) TEST_CHECK_STRING_N(String, Value, String.GetLength())

#define SUCCESS()                       \
    std::cout << "[TESTS SUCCEEDED]\n"; \
    return true