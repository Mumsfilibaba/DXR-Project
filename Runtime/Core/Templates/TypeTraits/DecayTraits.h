#pragma once
#include "Core/Templates/TypeTraits/AddTraits.h"
#include "Core/Templates/TypeTraits/Conditional.h"
#include "Core/Templates/TypeTraits/RemoveTraits.h"
#include "Core/Templates/TypeTraits/TypeCheckTraits.h"

// Decays T into the value that can be passed to a function by non-const/volatile value
// - T[N]     -> T*
// - const T& -> T
// - etc.

template<typename T>
struct TDecay
{
private:
    // Step 1: Remove any references from T to get U
    typedef typename TRemoveReference<T>::Type U;

    // Step 2: Prepare TrueType for when U is an array type. Remove one extent level from U (array to element type)
    // Then add a pointer to the element type.
    typedef typename TAddPointer<TRemoveExtent<U>>::Type TrueType;

    // Step 3: Prepare FalseType for when U is not an array. If U is a function type, remove CV-qualifiers and add 
    // a pointer to it. Otherwise, remove CV-qualifiers from U.
    typedef typename TConditional<
        TIsFunction<U>::Value,
        // If U is a function type, decay to pointer to unqualified function type
        typename TAddPointer<typename TRemoveCV<U>::Type>::Type,
        // If U is not a function type, remove CV-qualifiers from U
        typename TRemoveCV<U>::Type
    >::Type FalseType;

public:
    // Step 4: Determine the final decayed type If U is an array type, Type is TrueType (pointer to element type).
    // Otherwise, Type is FalseType (function pointer or unqualified type)
    typedef typename TConditional<TIsArray<U>::Value, TrueType, FalseType>::Type Type;
};
