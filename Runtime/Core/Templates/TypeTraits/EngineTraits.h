#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

// Checks if the type is a TArray-type(TArray, TArrayView, TStaticArray)
template<typename T>
struct TIsTArrayType : TFalseType { };

// Determine if this type is a string-type(TStaticString, TString, or TStringView)
template<typename T>
struct TIsTStringType : TFalseType { };
