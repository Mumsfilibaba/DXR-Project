#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Stores two variables of two template types

template<typename FirstType, typename SecondType>
struct TPair
{
    /* Defaults */
    TPair() = default;
    TPair(const TPair&) = default;
    TPair(TPair&&) = default;

    /**
     * Create a new instance of a pair
     * 
     * @param InFirst: Instance of the first type to copy
     * @param InSecond: Instance of the second type to copy
     */
    FORCEINLINE explicit TPair(const FirstType& InFirst, const SecondType& InSecond)
        : First(InFirst)
        , Second(InSecond)
    { }

    /**
     * Create a new instance of a pair
     *
     * @param InFirst: Instance of the first type to move
     * @param InSecond: Instance of the second type to move
     */
    template<typename OtherFirstType = FirstType, typename OtherSecondType = SecondType>
    FORCEINLINE explicit TPair(OtherFirstType&& InFirst, OtherSecondType&& InSecond)
        : First(Forward<OtherFirstType>(InFirst))
        , Second(Forward<OtherSecondType>(InSecond))
    { }

    /**
     * Copy constructor
     * 
     * @param Other: Pair to copy
     */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE explicit TPair(const TPair<OtherFirstType, OtherSecondType>& Other)
        : First(Other.First)
        , Second(Other.Second)
    { }

    /**
     * Move constructor
     *
     * @param Other: Pair to move
     */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE explicit TPair(TPair<OtherFirstType, OtherSecondType>&& Other)
        : First(Move(Other.First))
        , Second(Move(Other.Second))
    { }

    /**
     * Swap this pair with another
     * 
     * @param Other: Pair to swap with 
     */
    FORCEINLINE void Swap(TPair& Other) noexcept
    {
        ::Swap<FirstType>(First, Other.First);
        ::Swap<SecondType>(Second, Other.Second);
    }

public:

    /**
     * Copy-assignment operator
     * 
     * @param Rhs: Pair to copy
     * @return: A reference to this instance
     */
    FORCEINLINE TPair& operator=(const TPair& Rhs) noexcept
    {
        TPair(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Copy-assignment operator
     *
     * @param Rhs: Pair to copy
     * @return: A reference to this instance
     */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE TPair& operator=(const TPair<OtherFirstType, OtherSecondType>& Rhs) noexcept
    {
        First = Rhs.First;
        Second = Rhs.Second;
        return *this;
    }

    /**
     * Move-assignment operator
     *
     * @param Rhs: Pair to move
     * @return: A reference to this instance
     */
    FORCEINLINE TPair& operator=(TPair&& Rhs) noexcept
    {
        TPair(Move(Rhs)).Swap(*this);
        return *this;
    }

    /**
     * Move-assignment operator
     *
     * @param Rhs: Pair to move
     * @return: A reference to this instance
     */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE TPair& operator=(TPair<OtherFirstType, OtherSecondType>&& Rhs) noexcept
    {
        First = Move(Rhs.First);
        Second = Move(Rhs.Second);
        return *this;
    }

    FirstType First;
    SecondType Second;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Operators for TPair

template<typename FirstType, typename SecondType>
inline bool operator==(const TPair<FirstType, SecondType>& Lhs, const TPair<FirstType, SecondType>& Rhs) noexcept
{
    return (Lhs.First == Rhs.First) && (Lhs.Second == Rhs.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator!=(const TPair<FirstType, SecondType>& Lhs, const TPair<FirstType, SecondType>& Rhs) noexcept
{
    return !(Lhs == Rhs);
}

template<typename FirstType, typename SecondType>
inline bool operator<=(const TPair<FirstType, SecondType>& Lhs, const TPair<FirstType, SecondType>& Rhs) noexcept
{
    return (Lhs.First <= Rhs.First) && (Lhs.Second <= Rhs.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator<(const TPair<FirstType, SecondType>& Lhs, const TPair<FirstType, SecondType>& Rhs) noexcept
{
    return (Lhs.First < Rhs.First) && (Lhs.Second < Rhs.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator>=(const TPair<FirstType, SecondType>& Lhs, const TPair<FirstType, SecondType>& Rhs) noexcept
{
    return (Lhs.First >= Rhs.First) && (Lhs.Second >= Rhs.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator>(const TPair<FirstType, SecondType>& Lhs, const TPair<FirstType, SecondType>& Rhs) noexcept
{
    return (Lhs.First > Rhs.First) && (Lhs.Second > Rhs.Second);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Create helpers

template<typename FirstType, typename SecondType>
inline TPair<FirstType, SecondType> MakePair(const FirstType& First, const SecondType& Second) noexcept
{
    return TPair<FirstType, SecondType>(First, Second);
}

template<typename FirstType, typename SecondType>
inline TPair<FirstType, SecondType> MakePair(FirstType&& First, SecondType&& Second) noexcept
{
    return TPair<FirstType, SecondType>(Forward<FirstType>(First), Forward<SecondType>(Second));
}