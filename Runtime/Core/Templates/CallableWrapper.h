#pragma once

template<typename ReturnType, typename... ArgTypes>
struct TCallableWrapper
{
    using Type = ReturnType(ArgTypes...);
};

template<typename ReturnType>
struct TCallableWrapper<ReturnType, void>
{
    using Type = ReturnType();
};