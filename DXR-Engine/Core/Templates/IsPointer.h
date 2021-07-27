#pragma once

/* Check if type is pointer type */
template<typename T>
struct TIsPointer
{
    enum { Value = false };
};

template<typename T>
struct TIsPointer<T*>
{
    enum { Value = true };
};