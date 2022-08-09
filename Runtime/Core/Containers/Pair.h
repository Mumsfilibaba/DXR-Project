#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Stores two variables of two template types

template<
    typename FirstType,
    typename SecondType>
struct TPair
{
    /* Defaults */
    TPair() = default;
    TPair(const TPair&) = default;
    TPair(TPair&&) = default;

    /**
     * @brief: Create a new instance of a pair
     * 
     * @param InFirst: Instance of the first type to copy
     * @param InSecond: Instance of the second type to copy
     */
    FORCEINLINE explicit TPair(const FirstType& InFirst, const SecondType& InSecond)
        : First(InFirst)
        , Second(InSecond)
    { }

    /**
     * @brief: Create a new instance of a pair
     *
     * @param InFirst: Instance of the first type to move
     * @param InSecond: Instance of the second type to move
     */
    template<
        typename OtherFirstType  = FirstType,
        typename OtherSecondType = SecondType>
    FORCEINLINE explicit TPair(OtherFirstType&& InFirst, OtherSecondType&& InSecond)
        : First(Forward<OtherFirstType>(InFirst))
        , Second(Forward<OtherSecondType>(InSecond))
    { }

    /**
     * @brief: Copy constructor
     * 
     * @param Other: Pair to copy
     */
    template<
        typename OtherFirstType,
        typename OtherSecondType>
    FORCEINLINE explicit TPair(const TPair<OtherFirstType, OtherSecondType>& Other)
        : First(Other.First)
        , Second(Other.Second)
    { }

    /**
     * @brief: Move constructor
     *
     * @param Other: Pair to move
     */
    template<
        typename OtherFirstType,
        typename OtherSecondType>
    FORCEINLINE explicit TPair(TPair<OtherFirstType, OtherSecondType>&& Other)
        : First(Move(Other.First))
        , Second(Move(Other.Second))
    { }

    /**
     * @brief: Swap this pair with another
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
     * @brief: Copy-assignment operator
     * 
     * @param RHS: Pair to copy
     * @return: A reference to this instance
     */
    FORCEINLINE TPair& operator=(const TPair& RHS) noexcept
    {
        TPair(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Copy-assignment operator
     *
     * @param RHS: Pair to copy
     * @return: A reference to this instance
     */
    template<
        typename OtherFirstType,
        typename OtherSecondType>
    FORCEINLINE TPair& operator=(const TPair<OtherFirstType, OtherSecondType>& RHS) noexcept
    {
        First  = RHS.First;
        Second = RHS.Second;
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: Pair to move
     * @return: A reference to this instance
     */
    FORCEINLINE TPair& operator=(TPair&& RHS) noexcept
    {
        TPair(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: Pair to move
     * @return: A reference to this instance
     */
    template<
        typename OtherFirstType,
        typename OtherSecondType>
    FORCEINLINE TPair& operator=(TPair<OtherFirstType, OtherSecondType>&& RHS) noexcept
    {
        First  = Move(RHS.First);
        Second = Move(RHS.Second);
        return *this;
    }

    FirstType  First;
    SecondType Second;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Operators for TPair

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD bool operator==(const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS) noexcept
{
    return (LHS.First == RHS.First) && (LHS.Second == RHS.Second);
}

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD bool operator!=(const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD bool operator<=(const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS) noexcept
{
    return (LHS.First <= RHS.First) && (LHS.Second <= RHS.Second);
}

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD bool operator<(const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS) noexcept
{
    return (LHS.First < RHS.First) && (LHS.Second < RHS.Second);
}

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD bool operator>=(const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS) noexcept
{
    return (LHS.First >= RHS.First) && (LHS.Second >= RHS.Second);
}

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD bool operator>(const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS) noexcept
{
    return (LHS.First > RHS.First) && (LHS.Second > RHS.Second);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Create helpers

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD TPair<FirstType, SecondType> MakePair(const FirstType& First, const SecondType& Second) noexcept
{
    return TPair<FirstType, SecondType>(First, Second);
}

template<
    typename FirstType,
    typename SecondType>
inline NODISCARD TPair<FirstType, SecondType> MakePair(FirstType&& First, SecondType&& Second) noexcept
{
    return TPair<FirstType, SecondType>(Forward<FirstType>(First), Forward<SecondType>(Second));
}