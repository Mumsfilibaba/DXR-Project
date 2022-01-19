#pragma once
#include "IsSame.h"
#include "RemoveCV.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Custom nullptr type

typedef decltype(nullptr) NullptrType;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determines weather type is nullptr or not

template<typename T>
struct TIsNullptr
{
    enum
    {
        Value = TIsSame<NullptrType, typename TRemoveCV<T>::Type>::Value
    };
};