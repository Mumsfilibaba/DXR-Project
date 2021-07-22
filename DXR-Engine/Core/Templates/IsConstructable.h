#pragma once

/* Checks weather the type can be constructed with the specified arguments */
template<typename T, typename ArgTypes>
struct TIsConstructable
{
    static constexpr bool Value = __is_constructable(T, ArgTypes...);
};