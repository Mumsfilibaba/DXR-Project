#pragma once
#include <iostream>

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

#define TEST_CHECK(bCondition)                                                        \
    if (!(bCondition))                                                                \
    {                                                                                 \
        std::cout << "[TEST FAILED] Condition='" << MAKE_STRING(bCondition) << "'\n"; \
        DEBUG_BREAK();                                                                \
        return false;                                                                 \
    }                                                                                 \
    else                                                                              \
    {                                                                                 \
        std::cout << "[TEST SUCCEEDED]: '" << MAKE_STRING(bCondition) << "'\n";       \
    }

#define TEST_CHECK_ARRAY(Array, ...)                                                                                             \
    if (Array.IsEmpty())                                                                                                         \
    {                                                                                                                            \
        std::cout << "[TEST FAILED] '" << MAKE_STRING(Array) "' is empty\n";                                                     \
        DEBUG_BREAK();                                                                                                           \
        return false;                                                                                                            \
    }                                                                                                                            \
    else                                                                                                                         \
    {                                                                                                                            \
        std::initializer_list InitList = MAKE_VAR(__VA_ARGS__);                                                                  \
                                                                                                                                 \
        int32 Index = 0;                                                                                                         \
        for (auto Element : InitList)                                                                                            \
        {                                                                                                                        \
            if ((Index >= static_cast<int32>(Array.Size())) || (Array[Index] != Element))                                     \
            {                                                                                                                    \
                std::cout << "[TEST FAILED] '" << MAKE_STRING(Array) << '[' << Index << ']' << " != " << Element[Index] << '\n'; \
                DEBUG_BREAK();                                                                                                   \
                return false;                                                                                                    \
            }                                                                                                                    \
                                                                                                                                 \
            Index++;                                                                                                             \
        }                                                                                                                        \
                                                                                                                                 \
        std::cout << "[TEST SUCCEEDED]: '" << MAKE_STRING(Array) << "' == " << MAKE_STRING(__VA_ARGS__) << '\n';                 \
    }

#define TEST_CHECK_STRING_N(String, Value, NumChars)                                                  \
    {                                                                                                 \
        const bool bResult = (String.Length() == NumChars) &&                                         \
            TCString<decltype(String)::CHARTYPE>::Strncmp(String.GetCString(), Value, NumChars) == 0; \
        if (!bResult)                                                                                 \
        {                                                                                             \
            std::cout << "[TEST FAILED] Condition=" << #String << "='";                               \
            PrintString<decltype(String)::CHARTYPE>(String.GetCString());                             \
            std::cout << "'\n";                                                                       \
            std::cout << #String << "='";                                                             \
            PrintString<decltype(String)::CHARTYPE>(Value);                                           \
            std::cout << "'\n";                                                                       \
            DEBUG_BREAK();                                                                            \
            return false;                                                                             \
        }                                                                                             \
        else                                                                                          \
        {                                                                                             \
            std::cout << "[TEST SUCCEEDED]: " << #String << "='";                                     \
            PrintString<decltype(String)::CHARTYPE>(String.GetCString());                             \
            std::cout << "'\n";                                                                       \
        }                                                                                             \
    }

#define TEST_CHECK_STRING(String, Value) TEST_CHECK_STRING_N(String, Value, TCString<decltype(String)::CHARTYPE>::Strlen(Value))

#define SUCCESS()                       \
    std::cout << "[TESTS SUCCEEDED]\n"; \
    return true