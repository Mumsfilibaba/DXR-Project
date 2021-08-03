#pragma once
#include "IsArithmetic.h"

/* Checks if the type is signed */
template<typename T>
struct TIsSigned
{
private:
    template<typename T, bool = TIsArithmetic<T>::Value>
    struct TIsSignedImpl
    {
        enum
        {
            Value = T( -1 ) < T( 0 )
        };
    };


    template<typename T>
    struct TIsSignedImpl<T, false>
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
    template<typename T, bool = TIsArithmetic<T>::Value>
    struct TIsUnsignedImpl
    {
        enum
        {
            Value = T( 0 ) < T( -1 )
        };
    };


    template<typename T>
    struct TIsUnsignedImpl<T, false>
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