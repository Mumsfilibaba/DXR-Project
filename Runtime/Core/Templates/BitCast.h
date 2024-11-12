#pragma once
#include "TypeTraits.h"

template <typename T, typename F>
constexpr typename TEnableIf<TAnd<TValue<sizeof(T) == sizeof(F)>, TIsTriviallyCopyable<F>, TIsTriviallyCopyable<T>>::Value, T>::Type BitCast(const F& Src) noexcept
{
    static_assert(sizeof(T) == sizeof(F), "Sizes must be equal");
    static_assert(TIsTriviallyCopyable<F>::Value, "F must be trivially copyable");
    static_assert(TIsTriviallyCopyable<T>::Value, "T must be trivially copyable");

    union
    {
        F Src;
        T Dst;
    } UnionCast = { Src };
    return UnionCast.Dst;
}
