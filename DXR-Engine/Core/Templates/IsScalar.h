#pragma once
#include "IsArithmetic.h"
#include "IsEnum.h"
#include "IsPointer.h"
#include "IsMemberPointer.h"
#include "IsNullptr.h"
#include "Or.h"

/* Check if a type is a scalar */
template<typename T>
struct TIsScalar
{
    static constexpr bool Value = TOr<TIsArithmetic<T>, TIsEnum<T>, TIsPointer<T>, TIsMemberPointer<T>, TIsNullptr<T>>::Value;
};