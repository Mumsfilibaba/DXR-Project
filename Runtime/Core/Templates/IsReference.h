#pragma once
#include "Or.h"

/* Check if type is lvalue reference type */
template<typename T>
struct TIsLValueReference
{
    enum
    {
        Value = false
    };
};

template<typename T>
struct TIsLValueReference<T&>
{
    enum
    {
        Value = true
    };
};

/* Check if type is rvalue reference type */
template<typename T>
struct TIsRValueReference
{
    enum
    {
        Value = false
    };
};

template<typename T>
struct TIsRValueReference<T&&>
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
        Value = TOr<TIsLValueReference<T>, TIsRValueReference<T>>::Value
    };
};