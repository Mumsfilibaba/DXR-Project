#pragma once
#include "AddReference.h"
#include "IsConstructible.h"
#include "IsAssignable.h"

template<typename T>
struct TIsCopyConstructable
{
    enum { Value = TIsConstructible<T, typename TAddLValueReference<const T>::Type>::Value };
};

template<typename T>
struct TIsCopyAssignable
{
    enum { Value = TIsAssignable<T, typename TAddLValueReference<const T>::Type>::Value };
};