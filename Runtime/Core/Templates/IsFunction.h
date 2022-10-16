#pragma once
#include "Core/CoreDefines.h"
#include "IsConst.h"
#include "IsReference.h"

#if PLATFORM_COMPILER_MSVC
#pragma warning(push)
#pragma warning( disable : 4180 )
#endif

template<typename T>
struct TIsFunction
{
    /* Functions and references cannot be const */
    enum { Value = (!TIsConst<const T>::Value) && (!TIsReference<T>::Value) };
};

#if PLATFORM_COMPILER_MSVC
    #pragma warning(pop)
#endif