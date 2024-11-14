#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

template<typename T>
struct TIsConst : TFalseType { };

template<typename T>
struct TIsConst<const T> : TTrueType { };

template<typename T>
struct TIsVolatile : TFalseType { };

template<typename T>
struct TIsVolatile<volatile T> : TTrueType { };