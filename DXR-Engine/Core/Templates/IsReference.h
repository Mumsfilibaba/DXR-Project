#pragma once

template<typename T>
struct TIsLValueReference
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsLValueReference<T&>
{
    static constexpr bool Value = true;
};

/* Check if type is lvalue reference type */
template<typename T>
inline constexpr bool IsLValueReference = TIsLValueReference<T>::Value;

template<typename T>
struct TIsRValueReference
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsRValueReference<T&&>
{
    static constexpr bool Value = true;
};

/* Check if type is rvalue reference type */
template<typename T>
inline constexpr bool IsRValueReference = TIsRValueReference<T>::Value;