#pragma once
#include "Core/Templates/Utility/MoveSemantics.h"

// Swap Elements
template<typename T>
constexpr void Swap(T& LHS, T& RHS) noexcept(TAnd<TIsNothrowMoveConstructable<T>, TIsNothrowMoveAssignable<T>>::Value) requires (TNot<TIsConst<T>>::Value)
{
    T TempElement = Move(LHS);
    LHS = Move(RHS);
    RHS = Move(TempElement);
}