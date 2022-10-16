#pragma once
#include "IsScalar.h"
#include "IsArray.h"
#include "IsUnion.h"
#include "IsClass.h"
#include "Or.h"

template<typename T>
struct TIsObject
{
    enum { Value = TOr<TIsScalar<T>, TIsArray<T>, TIsUnion<T>, TIsClass<T>>::Value };
};