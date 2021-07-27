#pragma once

/* Value returns the and-ed value of all types value */
template<typename... ArgsType>
struct TAnd;

template<typename T, typename... ArgsType>
struct TAnd<T, ArgsType...>
{
    enum { Value = (T::Value && TAnd<ArgsType...>::Value) };
};

template<typename T>
struct TAnd<T>
{
    enum { Value = T::Value };
};