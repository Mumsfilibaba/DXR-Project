#pragma once
#include "Core/Templates/TypeTraits/BasicTraits.h"

typedef TIntegralConstant<bool, true>  TTrueType;
typedef TIntegralConstant<bool, false> TFalseType;

template<bool bIdentityValue>
struct TValue
{
    static constexpr bool Value = bIdentityValue;
};

template<typename T>
struct TNot
{
    static constexpr bool Value = !T::Value;
};

template<typename... ArgsType>
struct TAnd;

template<typename T, typename... ArgsType>
struct TAnd<T, ArgsType...>
{
    static constexpr bool Value = (T::Value && TAnd<ArgsType...>::Value);
};

template<typename T>
struct TAnd<T>
{
    static constexpr bool Value = T::Value;
};

template<typename... ArgsType>
struct TOr;

template<typename T, typename... ArgsType>
struct TOr<T, ArgsType...>
{
    static constexpr bool Value = (T::Value || TOr<ArgsType...>::Value);
};

template<typename T>
struct TOr<T>
{
    static constexpr bool Value = T::Value;
};

template<typename... ArgsType>
struct TNand
{
    static constexpr bool Value = !TAnd<ArgsType...>::Value;
};

template<typename... ArgsType>
struct TNor
{
    static constexpr bool Value = !TOr<ArgsType...>::Value;
};

template<typename... Args>
struct TXor;

template<typename T, typename... Args>
struct TXor<T, Args...>
{
    static constexpr bool Value = T::Value != TXor<Args...>::Value;
};

template<typename T>
struct TXor<T>
{
    static constexpr bool Value = T::Value;
};

template<>
struct TXor<>
{
    static constexpr bool Value = false;
};

template<int64 First, int64... Values>
struct TMin;

template<int64 InValue>
struct TMin<InValue>
{
    static constexpr int64 Value = InValue;
};

template<int64 First, int64 Second, int64... Rest>
struct TMin<First, Second, Rest...>
{
    static constexpr int64 Value = TMin<(First <= Second ? First : Second), Rest...>::Value;
};

template<int64 First, int64... Values>
struct TMax;

template<int64 InValue>
struct TMax<InValue>
{
    static constexpr int64 Value = InValue;
};

template<int64 First, int64 Second, int64... Rest>
struct TMax<First, Second, Rest...>
{
    static constexpr int64 Value = TMax<(First >= Second ? First : Second), Rest...>::Value;
};

template<int64 Value, int64 Min, int64 Max>
struct TClamp
{
    static_assert(Min <= Max, "Min must be less than or equal to Max");

    // First, clamp Value to be at least Min using TMax
    // Then, clamp the result to be at most Max using TMin
    static constexpr int64 Result = TMin<TMax<Value, Min>::Value, Max>::Value;
};

template<int64... Values>
struct TAdd;

template<int64 First, int64... Rest>
struct TAdd<First, Rest...>
{
    static constexpr int64 Value = First + TAdd<Rest...>::Value;
};

template<int64 InValue>
struct TAdd<InValue>
{
    static constexpr int64 Value = InValue;
};

template<>
struct TAdd<>
{
    static constexpr int64 Value = 0;
};

template<int64... Values>
struct TSub;

template<int64 First, int64... Rest>
struct TSub<First, Rest...>
{
    static constexpr int64 Value = First - TSub<Rest...>::Value;
};

template<int64 InValue>
struct TSub<InValue>
{
    static constexpr int64 Value = InValue;
};

template<>
struct TSub<>
{
    static constexpr int64 Value = 0;
};

template<int64 N>
struct TAbs
{
    static constexpr int64 Value = (N < 0) ? -N : N;
};

template<int64 N>
struct TIsEven
{
    static constexpr bool Value = (N % 2) == 0;
};

template<int64 N>
struct TIsOdd
{
    static constexpr bool Value = (N % 2) != 0;
};

template<int64 Base, int64 Exponent>
struct TPow
{
    static_assert(Exponent >= 0, "Exponent must be non-negative");
    static constexpr int64 Value = Base * TPow<Base, Exponent - 1>::Value;
};

template<int64 Base>
struct TPow<Base, 0>
{
    static constexpr int64 Value = 1;
};

template<int64 N>
struct TFactorial
{
    static_assert(N >= 0, "N must be non-negative");
    static constexpr int64 Value = N * TFactorial<N - 1>::Value;
};

template<>
struct TFactorial<0>
{
    static constexpr int64 Value = 1;
};