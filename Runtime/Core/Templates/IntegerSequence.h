#pragma once
#include "Core/Core.h"

template <typename T, T... Sequence>
struct TIntegerSequence
{
    static_assert(TIsInteger<T>::Value, "TIntegerSequence must an integral type");

    typedef T Type;
 
    inline static constexpr auto Size = sizeof...(Sequence);
};


namespace IntegerSequenceInternal
{
    template <typename T, unsigned N>
    struct TMakeIntegerSequenceImpl;
}

template<typename T, T N>
using TMakeIntegerSequence = typename IntegerSequenceInternal::TMakeIntegerSequenceImpl<T, N>::Type;


namespace IntegerSequenceInternal
{
    template<uint32 N, typename FirstSequence, typename SecondSequence>
    struct TIntegerSequenceHelper;

    template<uint32 N, typename T, T... First, T... Second>
    struct TIntegerSequenceHelper<N, TIntegerSequence<T, First...>, TIntegerSequence<T, Second...>> : TIntegerSequence<T, First..., (T(N + Second))...>
    {
        using Type = TIntegerSequence<T, First..., (T(N + Second))...>;
    };
    
    template<uint32 N, typename FirstSequence, typename SecondSequence>
    using TIntegerSequenceHelperType = typename TIntegerSequenceHelper<N, FirstSequence, SecondSequence>::Type;

    template<typename T, uint32 N>
    struct TMakeIntegerSequenceImpl : TIntegerSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>
    {
        using Type = TIntegerSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>;
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