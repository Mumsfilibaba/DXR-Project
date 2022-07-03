#pragma once
#include "Core/CoreTypes.h"

#include <initializer_list>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// std::initializer_list helpers

template<typename T>
CONSTEXPR T* GetInitializerListData(std::initializer_list<T> InList)
{
    return const_cast<T*>(InList.begin());
}

template<typename T>
CONSTEXPR int32 GetInitializerListSize(std::initializer_list<T> InList)
{
    return static_cast<int32>(InList.size());
}