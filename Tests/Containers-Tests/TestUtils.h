#pragma once
#include <iostream>
#include <cassert>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper macro for tests

#define MAKE_STRING(...) #__VA_ARGS__

#define MAKE_VAR(...) __VA_ARGS__

#define CHECK(bCondition)                                                             \
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

#define CHECK_ARRAY(Array, ...)                                                                                                  \
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
            if ((Index >= static_cast<int32>(Array.Size())) || (Array[Index] != Element))                                        \
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

#define SUCCESS()                             \
    std::cout << "[TESTS SUCCEEDED]" << '\n'; \
    return true;