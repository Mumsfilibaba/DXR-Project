#pragma once
#include "Identity.h"
#include "RemoveReference.h"

template<typename T>
struct TAddPointer
{
    typedef typename TIdentity<typename TRemoveReference<T>::Type*>::Type Type;
};