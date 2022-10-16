#pragma once
#include "Or.h"
#include "IsInteger.h"
#include "IsFloatingPoint.h"

template<typename T>
struct TIsArithmetic
{
    enum { Value = TOr<TIsInteger<T>, TIsFloatingPoint<T>>::Value };
};