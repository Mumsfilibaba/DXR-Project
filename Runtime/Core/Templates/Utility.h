#pragma once
#include "AddressOf.h"
#include "AlignedBytes.h"
#include "BitCast.h"
#include "IntegerSequence.h"
#include "EnumOperators.h"
#include "FunctionHelpers.h"
#include "InPlaceHelpers.h"
#include "MoveSemantics.h"
#include "NonCopyable.h"
#include "TypeTraits.h"

// Swap Elements
template<typename T>
constexpr void Swap(T& LHS, T& RHS) noexcept requires(TNot<TIsConst<T>>::Value)
{
    T TempElement = Move(LHS);
    LHS = Move(RHS);
    RHS = Move(TempElement);
}

template<typename T>
constexpr uintptr ToInteger(T Pointer) requires(TIsPointer<T>::Value)
{
    return reinterpret_cast<uintptr>(Pointer);
}

template<typename EnumType>
constexpr typename TUnderlyingType<EnumType>::Type ToUnderlying(EnumType Value)
{
    return static_cast<typename TUnderlyingType<EnumType>::Type>(Value);
}

template<typename T>
typename TAddReference<T>::RValue DeclVal() noexcept;

template<typename... Packs>
inline void ExpandPacks(Packs&&...) { }
