#pragma once

template<typename T>
struct _TRemoveReference
{
    using Type = T;
};

template<typename T>
struct _TRemoveReference<T&>
{
    using Type = T;
};

template<typename T>
struct _TRemoveReference<T&&>
{
    using Type = T;
};

/* Removes reference and retrives the types */
template<typename T>
using TRemoveReference = typename _TRemoveReference<T>::Type;