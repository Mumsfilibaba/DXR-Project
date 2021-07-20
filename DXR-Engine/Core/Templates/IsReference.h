#pragma once
#include "Or.h"

/* Check if type is lvalue reference type */
template<typename T>
struct TIsLeftValueReference
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsLeftValueReference<T&>
{
    static constexpr bool Value = true;
};

/* Check if type is rvalue reference type */
template<typename T>
struct TIsRightValueReference
{
    static constexpr bool Value = false;
};

template<typename T>
struct TIsRightValueReference<T&&>
{
    static constexpr bool Value = true;
};

/* Check if type is either lvalue- or rvalue reference */
template<typename T>
struct TIsReference
{
    static constexpr bool Value = TOr<TIsLeftValueReference<T>, TIsRightValueReference<T>>::Value;
};