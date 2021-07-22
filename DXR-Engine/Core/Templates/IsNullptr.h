#pragma once
#include "IsSame.h"
#include "RemoveCV.h"

/* Declare custom nullptr type */
typedef decltype(nullptr) NullptrType;

/* Determines weather type is nullptr or not */
template<typename T>
struct TIsNullptr
{
    static constexpr bool Value = TIsSame<NullptrType, typename TRemoveCV<T>::Type >> ::Value;
};