#pragma once
#include "AddReference.h"
#include "IsConstructible.h"
#include "IsAssignable.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Checks weather the type can be constructed with a move constructor

template<typename T>
struct TIsMoveConstructable
{
    enum
    {
        Value = TIsConstructible<T, typename TAddRValueReference<T>::Type>::Value
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Checks weather the type can be constructed with a move assignment operator

template<typename T>
struct TIsMoveAssignable
{
    enum
    {
        Value = TIsAssignable<T, typename TAddRValueReference<T>::Type>::Value
    };
};