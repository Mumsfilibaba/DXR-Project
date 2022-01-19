#pragma once
#include "AddReference.h"
#include "IsConstructible.h"
#include "IsAssignable.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Checks weather the type can be constructed with a copy constructor

template<typename T>
struct TIsCopyConstructable
{
    enum
    {
        Value = TIsConstructible<T, typename TAddLValueReference<const T>::Type>::Value
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Checks weather the type can be constructed with a copy assignment operator

template<typename T>
struct TIsCopyAssignable
{
    enum
    {
        Value = TIsAssignable<T, typename TAddLValueReference<const T>::Type>::Value
    };
};