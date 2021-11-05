#pragma once
#include "Identity.h"

template<typename T>
struct TUnderlyingType
{
    typedef TIdentity<__underlying_type(T)>::Type Type;
};