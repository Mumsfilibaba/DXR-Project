#pragma once
#include "Identity.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Retrieve the underlying type

template<typename T>
struct TUnderlyingType
{
    typedef typename TIdentity<__underlying_type(T)>::Type Type;
};
