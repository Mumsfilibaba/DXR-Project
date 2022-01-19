#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Removes reference and retrieves the types

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