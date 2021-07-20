#pragma once
#include "IsArithmetic.h"

/* Checks if the type is signed */
template<typename T>
struct TIsSigned
{
    static constexpr bool Value = TIsArithmetic<T>::Value && (T( -1 ) < T( 0 ));
};

/* Checks if the type is unsigned */
template<typename T>
struct TIsUnsigned
{
    static constexpr bool Value = TIsArithmetic<T>::Value && (T( 0 ) < T( -1 ));
};