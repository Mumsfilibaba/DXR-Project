#pragma once

/* Removes reference and retrives the types */
template<typename T>
struct TRemoveReference
{
    typedef T Type;
};

template<typename T>
struct TRemoveReference<T&>
{
    typedef T Type;
};

template<typename T>
struct TRemoveReference<T&&>
{
    typedef T Type;
};