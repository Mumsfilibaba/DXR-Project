#pragma once
#include "IsVoid.h"
#include "IsArithmetic.h"
#include "IsNullptr.h"

/* Determines if the type is fundamental or not */
template<typename T>
struct TIsFundamental
{
    static constexpr bool Value = TOr<typename TIsArithmetic<T>, typename TIsVoid<T>, typename TIsNullptr<T>>::Value;
};

/* Determines if the type is compond or not */
template<typename T>
struct TIsCompond
{
    static constexpr bool Value = !TIsFundamental<T>::Value;
};