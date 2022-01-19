#pragma once
#include "Core/CoreDefines.h"
#include "IsConst.h"
#include "IsReference.h"

#if COMPILER_MSVC
#pragma warning(push)
#pragma warning( disable : 4180 )
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Determine if the type is a function

template<typename T>
struct TIsFunction
{
    enum
    {
        /* Functions and references cannot be are not const */
        Value = (!TIsConst<const T>::Value) && (!TIsReference<T>::Value)
    };
};

#if COMPILER_MSVC
#pragma warning(pop)
#endif