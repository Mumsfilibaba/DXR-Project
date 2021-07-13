#pragma once

template<bool TCondition, typename T = void>
struct _TEnableIf
{
};

template<typename T>
struct _TEnableIf<true, T>
{
    typedef T Type;
};

/* Enables return value if condition is met */
template<bool TCondition, typename T = void>
using TEnableIf = typename _TEnableIf<TCondition, T>::Type;

