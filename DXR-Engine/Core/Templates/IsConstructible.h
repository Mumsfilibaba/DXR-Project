#pragma once

/* Checks weather the type can be constructed with the specified arguments */
template<typename T, typename ArgTypes>
struct TIsConstructible
{
    static constexpr bool Value = __is_constructible( T, ArgTypes... );
};