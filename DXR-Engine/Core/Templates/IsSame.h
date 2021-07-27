#pragma once

/* Checks if two types are the same */
template<typename T, typename U>
struct TIsSame
{
    enum { Value = false };
};

template<typename T>
struct TIsSame<T, T>
{
    enum { Value = true };
};
