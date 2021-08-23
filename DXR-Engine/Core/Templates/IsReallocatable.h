#pragma once
#include "IsTrivial.h"

/* Determine if the type can be reallocated using realloc, that is the type does not reference itself or have classes pointing directly to an element */
template<typename T>
struct TIsReallocatable
{
    enum
    {
        Value = TIsTrivial<T>::Value
    };
};