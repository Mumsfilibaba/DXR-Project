#pragma once
#include "Core/Core.h"

template <typename T, T... Sequence>
struct TIntegerSequence
{
    static_assert(TIsInteger<T>::Value, "TIntegerSequence must an integral type");

    typedef T Type;

    inline static constexpr SIZE_T Size = sizeof...(Sequence);
};

namespace IntegerSequenceInternal
{
    template<typename T, SIZE_T N>
    struct TMakeIntegerSequenceImpl;

    template<typename T, SIZE_T N, typename F, typename S>
    struct TIntegerSequenceImpl;
}

template<typename T, T N>
using TMakeIntegerSequence = typename IntegerSequenceInternal::TMakeIntegerSequenceImpl<T, N>::Type;

namespace IntegerSequenceInternal
{
    template<typename T, SIZE_T N, typename F, typename S>
    struct TIntegerSequenceImpl;

    template<typename T, SIZE_T N, T... First, T... Second>
    struct TIntegerSequenceImpl<T, N, TIntegerSequence<T, First...>, TIntegerSequence<T, Second...>> : TIntegerSequence<T, First..., (T(N + Second))...>
    {
        using Type = TIntegerSequence<T, First..., (T(N + Second))...>;
    };

    template<typename T, SIZE_T N, typename F, typename S>
    using TIntegerSequenceImplType = typename TIntegerSequenceImpl<T, N, F, S>::Type;

    template<typename T, SIZE_T N>
    struct TMakeIntegerSequenceImpl : TIntegerSequenceImplType<T, N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>
    {
        using Type = TIntegerSequenceImplType<T, N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>;
    };

    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 1> : TIntegerSequence<T, T(0)>
    {
        using Type = TIntegerSequence<T, T(0)>;
    };

    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 0> : TIntegerSequence<T>
    {
        using Type = TIntegerSequence<T>;
    };
}
