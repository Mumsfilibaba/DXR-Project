#pragma once
#include "Core/CoreTypes.h"
#include "IsInteger.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIntegerSequence

template <
    typename T,
    T... Sequence>
struct TIntegerSequence
{
    static_assert(TIsInteger<T>::Value, "TIntegerSequence must an integral type");

    typedef T Type;
 
    enum { Size = sizeof...(Sequence) };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMakeIntegerSequenceImpl

namespace Internal
{
    template <
        typename T,
        unsigned N>
    struct TMakeIntegerSequenceImpl;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMakeIntegerSequence

template<
    typename T,
    T N>
using TMakeIntegerSequence = typename Internal::TMakeIntegerSequenceImpl<T, N>::Type;

namespace Internal
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TSequenceHelper

    template<
        uint32 N,
        typename FirstSequence,
        typename SecondSequence>
    struct TSequenceHelper;

    template<
        uint32 N,
        typename T,
        T... First,
        T... Second>
    struct TSequenceHelper<N, TIntegerSequence<T, First...>, TIntegerSequence<T, Second...>> 
        : TIntegerSequence<T, First..., (T(N + Second))...>
    {
        using Type = TIntegerSequence<T, First..., (T(N + Second))...>;
    };
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TSequenceHelperType

    template<
        uint32 N,
        typename FirstSequence,
        typename SecondSequence>
    using TSequenceHelperType = typename TSequenceHelper<N, FirstSequence, SecondSequence>::Type;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // TMakeIntegerSequenceImpl

    template<
        typename T,
        uint32 N>
    struct TMakeIntegerSequenceImpl 
        : TSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>
    {
        using Type = TSequenceHelperType<N / 2, TMakeIntegerSequence<T, N / 2>, TMakeIntegerSequence<T, N - N / 2>>;
    };

    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 1> 
        : TIntegerSequence<T, T(0)>
    {
        using Type = TIntegerSequence<T, T(0)>;
    };

    template<typename T>
    struct TMakeIntegerSequenceImpl<T, 0> 
        : TIntegerSequence<T>
    {
        using Type = TIntegerSequence<T>;
    };
}