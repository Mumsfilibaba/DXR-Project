#pragma once

template<bool Condition, typename T = void>
struct _TEnableIf
{
};

template<typename T>
struct _TEnableIf<true, T>
{
    typedef T Type;
};

/* Enables return value if condition is met */
template<bool Condition, typename T = void>
using TEnableIf = typename _TEnableIf<Condition, T>::Type;

