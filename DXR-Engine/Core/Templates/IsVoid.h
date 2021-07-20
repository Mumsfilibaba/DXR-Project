#pragma once
#include "IsSame.h"
#include "RemoveCV.h>

/* Determine if the type is void */
template<typename T>
struct TIsVoid
{
    static constexpr bool Value = TIsSame<void, typename TRemoveCV<T>::Type >> ::Value;
};