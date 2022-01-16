#pragma once
#include "IsArithmetic.h"

/* Checks if the type is signed */
template<typename T>
struct TIsSigned
{
private:
    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsSignedImpl
    {
        enum
        {
            Value = U(-1) < U(0)
        };
    };


    template<typename U>
    struct TIsSignedImpl<U, false>
    {
        enum
        {
            Value = false
        };
    };

public:
    enum
    {
        Value = TIsSignedImpl<T>::Value
    };
};

/* Checks if the type is unsigned */
template<typename T>
struct TIsUnsigned
{
private:
    template<typename U, bool = TIsArithmetic<U>::Value>
    struct TIsUnsignedImpl
    {
        enum
        {
            Value = U(0) < U(-1)
        };
    };


    template<typename U>
    struct TIsUnsignedImpl<U, false>
    {
        enum
        {
            Value = false
        };
    };

public:
    enum
    {
        Value = TIsUnsignedImpl<T>::Value
    };
};
