#pragma once

/* Value returns the or-ed value of all types value */
template<typename... ArgsType>
struct TOr;

template<typename T, typename... ArgsType>
struct TOr<T, ArgsType...>
{
    static constexpr bool Value = T::Value || TOr<ArgsType...>::Value;
};

template<typename T>
struct TOr<T>
{
    static constexpr bool Value = T::Value;
};