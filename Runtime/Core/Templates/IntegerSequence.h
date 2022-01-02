#pragma once
#include "Core/CoreTypes.h"
#include "IsInteger.h"

/* Implements integer sequence */
template <typename T, T... Sequence>
struct TIntegerSequence
{
    typedef T Type;

    static_assert(TIsInteger<T>::Value, "TIntegerSequence must an integral type");

    enum
    {
        Size = sizeof...(Sequence)
    };
};

/* Forward-declare helper */
namespace Internal
{
    template <typename T, unsigned N>
    struct TMakeIntegerSequenceImpl;
}

/* Create a integer sequence */
template<typename T, T N>
using TMakeIntegerSequence = typename Internal::TMakeIntegerSequenceImpl<T, N>::Type;

namespace Internal
{
    /* Helper to create a integer sequence */
    template<uint32 N, typename FirstSequence, typename SecondSequence>
    struct TSequenceHelper;

    template<uint32 N, typename T, T... First, T... Second>
    struct TSequenceHelper<N, TIntegerSequence<T, First...>, TIntegerSequence<T, Second...>> : TIntegerSequence<T, First..., (T( N + Second ))...>
    {
        using Type = TIntegerSequence<T, First..., (T( N + Second ))...>;
    };

    template<uint32 N, typename FirstSequence, typename SecondSequence>
    using TSequenceHelperType = typename TSequenceHelper<N, FirstSequence, SecondSequence>::Type;

    /* Create a integer sequence */
    template<typename T, uint32 N>
    struct TMakeIntegerSequenceImpl : TSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>
    {
        using Type = TSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>;
    };

    /* Create a integer sequence with zero */
    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 1> : TIntegerSequence<T, T( 0 )>
    {
        using Type = TIntegerSequence<T, T( 0 )>;
    };

    /* Create a empty integer sequence */
    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 0> : TIntegerSequence<T>
    {
        using Type = TIntegerSequence<T>;
    };
}