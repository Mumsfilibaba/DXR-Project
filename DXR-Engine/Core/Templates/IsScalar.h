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
    static constexpr bool Value = TOr<
        typename TIsArithmetic<T>, 
        typename TIsEnum<T>, 
        typename TIsPointer<T>, 
        typename TIsMemberPointer<T>, 
        typename TIsNullptr<T>>::Value;
};