#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"
#include "Core/Templates/TypeTraits/TypeCheckTraits.h"

// Determine if the type can be reallocated using realloc, that is the type does not reference itself
// or have classes pointing directly to an element. This also means that objects can be moved using 
// memmove without issues.

#define MARK_AS_REALLOCATABLE(Type) \
    template<> \
    struct TIsReallocatable<Type> : TTrueType { }

template<typename T>
struct TIsReallocatable
{
    static constexpr bool Value = TIsTrivial<T>::Value;
};
