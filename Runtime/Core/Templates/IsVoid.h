#pragma once
#include "IsSame.h"
#include "RemoveCV.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsVoid

template<typename T>
struct TIsVoid
{
    enum
    {
        Value = TIsSame<void, typename TRemoveCV<T>::Type>::Value
    };
};