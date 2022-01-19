#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Value returns the or-ed value of all types value

template<typename... ArgsType>
struct TOr;

template<typename T, typename... ArgsType>
struct TOr<T, ArgsType...>
{
    enum
    {
        Value = (T::Value || TOr<ArgsType...>::Value)
    };
};

template<typename T>
struct TOr<T>
{
    enum
    {
        Value = T::Value
    };
};