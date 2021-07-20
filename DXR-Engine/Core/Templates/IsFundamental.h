#pragma once
#include "IsVoid.h"
#include "IsArithmetic.h"
#include "IsNullptr.h"

/* Determines if the type is fundamental or not */
template<typename T>
struct TIsFundamental
{
    static constexpr bool Value = TOr<TIsArithmetic<T>, TIsVoid<T>, TIsNullptr<T>>::Value;
};

/* Determines if the type is compound or not */
template<typename T>
struct TIsCompound
{
    static constexpr bool Value = !TIsFundamental<T>::Value;
};