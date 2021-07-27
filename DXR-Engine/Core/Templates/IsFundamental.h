#pragma once
#include "IsVoid.h"
#include "IsArithmetic.h"
#include "IsNullptr.h"

/* Determines if the type is fundamental or not */
template<typename T>
struct TIsFundamental
{
    enum
    {
        Value = TOr<TIsArithmetic<T>, TIsVoid<T>, TIsNullptr<T>>::Value
    };
};

/* Determines if the type is compound or not */
template<typename T>
struct TIsCompound
{
    enum
    {
        Value = !TIsFundamental<T>::Value
    };
};