#pragma once
#include "Core/Templates/TypeTraits/AddTraits.h"

template<typename T>
typename TAddRValueReference<T>::Type DeclVal() noexcept;

template<typename... Packs>
inline void ExpandPacks(Packs&&...) { }
