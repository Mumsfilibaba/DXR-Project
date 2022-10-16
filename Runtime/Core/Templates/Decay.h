#pragma once
#include "Conditional.h"
#include "IsArray.h"
#include "IsFunction.h"
#include "AddPointer.h"
#include "RemoveCV.h"
#include "RemoveReference.h"
#include "RemoveExtent.h"

/**
 *  Decays T into the value that can be passed to a function by non-const/volatile value
 *  - T[N] -> T*
 *  - const T& -> T
 *  - etc.
 */

template<typename T>
struct TDecay
{
private:
    typedef typename TRemoveReference<T>::Type U;
    typedef typename TRemoveExtent<U>::Type* TrueType;
    typedef typename TConditional<TIsFunction<U>::Value, typename TAddPointer<U>::Type, typename TRemoveCV<U>::Type>::Type FalseType;

public:
    typedef typename TConditional<TIsArray<U>::Value, TrueType, FalseType>::Type Type;
};