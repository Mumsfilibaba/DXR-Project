#pragma once
#include "IsVoid.h"
#include "IsArithmetic.h"
#include "IsNullptr.h"

template<typename T>
struct TIsFundamental
{
    enum { Value = TOr<TIsArithmetic<T>, TIsVoid<T>, TIsNullptr<T>>::Value };
};

template<typename T>
struct TIsCompound
{
    enum { Value = !TIsFundamental<T>::Value };
};