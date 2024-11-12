#pragma once
#include "TypeTraits.h"

template<int32 InSize, int32 InAlignment>
struct TAlignedBytes
{
    ALIGN_AS(InAlignment) uint8 Data[InSize];
};

template<typename T>
using TTypeAlignedBytes = TAlignedBytes<sizeof(T), TAlignmentOf<T>::Value>;