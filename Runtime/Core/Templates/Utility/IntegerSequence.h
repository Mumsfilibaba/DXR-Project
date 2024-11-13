#pragma once
#include "Core/Core.h"

template <typename T, T... Sequence>
struct TIntegerSequence
{
    static_assert(TIsInteger<T>::Value, "TIntegerSequence must an integral type");

    typedef T Type;

    inline static constexpr TSIZE Size = sizeof...(Sequence);
};

namespace IntegerSequenceInternal
{
    template<typename T, TSIZE N>
    struct TMakeIntegerSequenceImpl;

    template<typename T, TSIZE N, typename F, typename S>
    struct TIntegerSequenceHelper;
}

template<typename T, T N>
using TMakeIntegerSequence = typename IntegerSequenceInternal::TMakeIntegerSequenceImpl<T, N>::Type;

namespace IntegerSequenceInternal
{
    template<typename T, TSIZE N, typename F, typename S>
    struct TIntegerSequenceHelper;

    template<typename T, TSIZE N, T... First, T... Second>
    struct TIntegerSequenceHelper<T, N, TIntegerSequence<T, First...>, TIntegerSequence<T, Second...>> : TIntegerSequence<T, First..., (T(N + Second))...>
    {
        using Type = TIntegerSequence<T, First..., (T(N + Second))...>;
    };

    template<typename T, TSIZE N, typename F, typename S>
    using TIntegerSequenceHelperType = typename TIntegerSequenceHelper<T, N, F, S>::Type;

    template<typename T, TSIZE N>
    struct TMakeIntegerSequenceImpl : TIntegerSequenceHelperType<T, N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>
    {
        using Type = TIntegerSequenceHelperType<T, N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>;
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
