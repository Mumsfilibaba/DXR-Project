#pragma once
#include "Core/Templates/TypeTraits/BooleanTraits.h"

template<typename T>
struct TIsArray : TFalseType { };

template<typename T>
struct TIsArray<T[]> : TTrueType { };

template<typename T, SIZE_T N>
struct TIsArray<T[N]> : TTrueType { };

template<typename T>
struct TIsBoundedArray : TFalseType { };

template<typename T, SIZE_T N>
struct TIsBoundedArray<T[N]> : TTrueType { };

template<typename T>
struct TIsUnboundedArray : TFalseType { };

template<typename T>
struct TIsUnboundedArray<T[]> : TTrueType { };

template<typename T>
struct TRank : TIntegralConstant<SIZE_T, 0> { };

template<typename T>
struct TRank<T[]> : TIntegralConstant<SIZE_T, TRank<T>::Value + 1> { };

template<typename T, SIZE_T N>
struct TRank<T[N]> : TIntegralConstant<SIZE_T, TRank<T>::Value + 1> { };

template<typename T, SIZE_T N = 0>
struct TExtent : TIntegralConstant<SIZE_T, 0> { };

template<typename T>
struct TExtent<T[], 0> : TIntegralConstant<SIZE_T, 0> { };

template<typename T, SIZE_T N>
struct TExtent<T[], N> : TExtent<T, N - 1> { };

template<typename T, SIZE_T I>
struct TExtent<T[I], 0> : TIntegralConstant<SIZE_T, I> { };

template<typename T, SIZE_T I, unsigned N>
struct TExtent<T[I], N> : TExtent<T, N - 1> { };

template<typename T>
struct TArrayElement
{
    typedef T Type;
};

template<typename T>
struct TArrayElement<T[]>
{
    typedef T Type;
};

template<typename T, SIZE_T N>
struct TArrayElement<T[N]>
{
    typedef T Type;
};

template<typename T>
struct TIsMultiDimensionalArray : TIntegralConstant<bool, (TRank<T>::Value > 1)> { };
