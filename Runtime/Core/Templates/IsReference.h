#pragma once
#include "Or.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsLValueReference

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsRValueReference

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsReference

template<typename T>
struct TIsReference
{
    enum
    {
        Value = TOr<TIsLValueReference<T>, TIsRValueReference<T>>::Value
    };
};