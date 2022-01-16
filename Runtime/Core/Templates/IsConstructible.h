#pragma once

/* Checks weather the type can be constructed with the specified arguments */
template<typename T, typename... ArgTypes>
struct TIsConstructible
{
    enum
    {
        Value = __is_constructible(T, ArgTypes...)
    };
};