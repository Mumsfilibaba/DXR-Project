#pragma once

template<typename T, typename U>
struct TIsSame
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsSame<T, T>
{
    static constexpr bool Value = true;
};

/* Checks if two types are the same */
template<typename T, typename U>
inline constexpr bool IsSame = TIsSame<T, U>::Value;

