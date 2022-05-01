#pragma once
#include "Identity.h"
#include "RemoveReference.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAddPointer

template<typename T>
struct TAddPointer
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type*>::Type Type;
};