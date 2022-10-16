#pragma once
#include "IsSame.h"
#include "RemoveCV.h"

typedef decltype(nullptr) nullptr_type;

template<typename T>
struct TIsNullptr
{
    enum { Value = TIsSame<nullptr_type, typename TRemoveCV<T>::Type>::Value };
};