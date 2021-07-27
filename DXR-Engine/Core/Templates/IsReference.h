#pragma once
#include "Or.h"

/* Check if type is lvalue reference type */
template<typename T>
struct TIsLeftValueReference
{
    enum
    {
        Value = false
    };
};

template<typename T>
struct TIsLeftValueReference<T&>
{
    enum
    {
        Value = true
    };
};

/* Check if type is rvalue reference type */
template<typename T>
struct TIsRightValueReference
{
    enum
    {
        Value = false
    };
};

template<typename T>
struct TIsRightValueReference<T&&>
{
    enum
    {
        Value = true
    };
};

/* Check if type is either lvalue- or rvalue reference */
template<typename T>
struct TIsReference
{
    enum
    {
        Value = TOr<TIsLeftValueReference<T>, TIsRightValueReference<T>>::Value
    };
};