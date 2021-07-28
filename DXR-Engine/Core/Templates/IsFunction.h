#pragma once
#include "IsConst.h"
#include "IsReference.h"

/* Determine if the type is a function */
template<typename T>
struct TIsFunction
{
    enum
    {
        /* Functions and references cannot be are not const */
        Value = (!TIsConst<const T>::Value) && (!TIsReference<T>::Value);
    };
};
