#pragma once
#include "IsConst.h"
#include "AddPointer.h"

#include "Core/CoreTypes.h"

#include <initializer_list>

template<typename T>
constexpr T* GetInitializerListData(std::initializer_list<T> InList)
{
    return const_cast<T*>(InList.begin());
}

template<typename T>
constexpr int32 GetInitializerListSize(std::initializer_list<T> InList)
{
    return static_cast<int32>(InList.size());
}