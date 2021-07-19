#pragma once

/* Enables return value if condition is met */
template<bool Condition, typename T = void>
struct TEnableIf
{
};

template<typename T>
struct TEnableIf<true, T>
{
    typedef T Type;
};
