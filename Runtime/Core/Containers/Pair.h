#pragma once
#include "CoreDefines.h"
#include "CoreTypes.h"

/* Stores two variables of two template types */
template<typename FirstType, typename SecondType>
struct TPair
{
    /* Defaults */
    TPair() = default;
    TPair( const TPair& ) = default;
    TPair( TPair&& ) = default;

    /* Init types */
    FORCEINLINE explicit TPair( const FirstType& InFirst, const SecondType& InSecond )
        : First( InFirst )
        , Second( InSecond )
    {
    }

    /* Init with rvalue types, with other types */
    template<typename OtherFirstType = FirstType, typename OtherSecondType = SecondType>
    FORCEINLINE explicit TPair( OtherFirstType&& InFirst, OtherSecondType&& InSecond )
        : First( Forward<OtherFirstType>( InFirst ) )
        , Second( Forward<OtherSecondType>( InSecond ) )
    {
    }

    /* Copy constructor */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE explicit TPair( const TPair<OtherFirstType, OtherSecondType>& Other )
        : First( Other.First )
        , Second( Other.Second )
    {
    }

    /* Move constructor */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE explicit TPair( TPair<OtherFirstType, OtherSecondType>&& Other )
        : First( Move( Other.First ) )
        , Second( Move( Other.Second ) )
    {
    }

    /* Swap two pairs */
    FORCEINLINE void Swap( TPair& Other ) noexcept
    {
        ::Swap<FirstType>( First, Other.First );
        ::Swap<SecondType>( Second, Other.Second );
    }

    /* Copy assignment */
    FORCEINLINE TPair& operator=( const TPair& Other ) noexcept
    {
        TPair( Other ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE TPair& operator=( const TPair<OtherFirstType, OtherSecondType>& Other ) noexcept
    {
        First = Other.First;
        Second = Other.Second;
        return *this;
    }

    /* Move assignment */
    FORCEINLINE TPair& operator=( TPair&& Other ) noexcept
    {
        TPair( Move( Other ) ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    template<typename OtherFirstType, typename OtherSecondType>
    FORCEINLINE TPair& operator=( TPair<OtherFirstType, OtherSecondType>&& Other ) noexcept
    {
        First = Move( Other.First );
        Second = Move( Other.Second );
        return *this;
    }

    FirstType First;
    SecondType Second;
};

/* Operators */
template<typename FirstType, typename SecondType>
inline bool operator==( const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS ) noexcept
{
    return (LHS.First == RHS.First) && (LHS.Second == RHS.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator!=( const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS ) noexcept
{
    return !(LHS == RHS);
}

template<typename FirstType, typename SecondType>
inline bool operator<=( const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS ) noexcept
{
    return (LHS.First <= RHS.First) && (LHS.Second <= RHS.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator<( const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS ) noexcept
{
    return (LHS.First < RHS.First) && (LHS.Second < RHS.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator>=( const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS ) noexcept
{
    return (LHS.First >= RHS.First) && (LHS.Second >= RHS.Second);
}

template<typename FirstType, typename SecondType>
inline bool operator>( const TPair<FirstType, SecondType>& LHS, const TPair<FirstType, SecondType>& RHS ) noexcept
{
    return (LHS.First > RHS.First) && (LHS.Second > RHS.Second);
}

/* Create a pair */
template<typename FirstType, typename SecondType>
inline TPair<FirstType, SecondType> MakePair( const FirstType& First, const SecondType& Second ) noexcept
{
    return TPair<FirstType, SecondType>( First, Second );
}

/* Create a pair */
template<typename FirstType, typename SecondType>
inline TPair<FirstType, SecondType> MakePair( FirstType&& First, SecondType&& Second ) noexcept
{
    return TPair<FirstType, SecondType>( Forward<FirstType>( First ), Forward<SecondType>( Second ) );
}