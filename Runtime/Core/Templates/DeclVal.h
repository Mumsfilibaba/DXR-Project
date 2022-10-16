#pragma once
#include "AddReference.h"

template<typename T>
typename TAddReference<T>::RValue DeclVal() noexcept;