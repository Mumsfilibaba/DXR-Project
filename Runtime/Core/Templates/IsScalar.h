#pragma once
#include "IsArithmetic.h"
#include "IsEnum.h"
#include "IsPointer.h"
#include "IsMemberPointer.h"
#include "IsNullptr.h"
#include "Or.h"

template<typename T>
struct TIsScalar
{
    enum { Value = TOr<TIsArithmetic<T>, TIsEnum<T>, TIsPointer<T>, TIsMemberPointer<T>, TIsNullptr<T>>::Value };
};