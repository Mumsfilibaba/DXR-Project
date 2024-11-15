#pragma once
#include "Core/Templates/Utility/MoveSemantics.h"
#include "Core/Templates/TypeTraits/BooleanTraits.h"
#include "Core/Templates/TypeTraits/MetaProgrammingFuncs.h"

// Swap Elements
template<typename T>
constexpr void Swap(T& LHS, T& RHS) noexcept(TAnd<TIsNothrowMoveConstructible<T>, TIsNothrowMoveAssignable<T>>::Value) requires (TNot<TIsConst<T>>::Value)
{
    T TempElement = Move(LHS);
    LHS = Move(RHS);
    RHS = Move(TempElement);
}

template<typename T>
struct TIsSwappable
{
private:

    // Attempt to call Swap(T&, T&) and check if it's a valid expression.
    template<typename U>
    static auto Test(int32) -> decltype(Swap(DeclVal<U&>(), DeclVal<U&>()), TTrueType());

    // Fallback if Swap(T&, T&) is not a valid expression.
    template<typename>
    static TFalseType Test(...);

public:

    // Value is true if swap is valid, false otherwise.
    static constexpr bool Value = decltype(Test<T>(0))::Value;
};

template<typename T>
struct TIsNothrowSwappable
{
    static constexpr bool Value = TIsSwappable<T>::Value && noexcept(Swap(DeclVal<T&>(), DeclVal<T&>()));
};
