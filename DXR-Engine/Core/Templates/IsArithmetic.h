#pragma once
#include "Or.h"
#include "RemoveCV.h"
#include "IsInteger.h"
#include "IsFloatingPoint.h"

/* Determine if the type is arthemic or not */
template<typename T>
struct TIsArithmetic
{
    static constexpr bool Value = TOr<TIsInteger<T>, TIsFloatingPoint<T>> > ::Value;
};