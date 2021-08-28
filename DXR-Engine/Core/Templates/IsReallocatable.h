#pragma once
#include "IsTrivial.h"

/* Determine if the type can be reallocated using realloc, that is the type does 
   not reference itself or have classes pointing directly to an element. This 
   also means that objects can be memmove:ed without issues. */
template<typename T>
struct TIsReallocatable
{
    enum
    {
        Value = TIsTrivial<T>::Value
    };
};