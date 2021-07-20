#pragma once
#include "IsScalar.h"
#include "IsArray.h"
#include "IsUnion.h"
#include "IsClass.h"
#include "Or.h"

/* Determine if the type is an object */
template<typename T>
struct TIsObject
{
    static constexpr bool Value = TOr<
        typename TIsScalar<T>, 
        typename TIsArray<T>, 
        typename TIsUnion<T>, 
        typename TIsClass<T>>::Value;
};