#pragma once
#include "Identity.h"
#include "RemoveReference.h"

/* Adds a pointer to the type */
template<typename T>
struct TAddPointer
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type*>::Type Type;
};