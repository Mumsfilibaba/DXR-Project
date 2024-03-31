#pragma once
#include "BitCast.h"
#include "Core/Core.h"

constexpr uint64 HashType(bool bValue)
{
    return static_cast<uint64>(bValue);
}

constexpr uint64 HashType(int8 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(uint8 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(int16 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(uint16 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(int32 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(uint32 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(int64 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(uint64 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 HashType(float Value)
{
    return static_cast<uint64>(BitCast<uint32>(Value));
}

constexpr uint64 HashType(double Value)
{
    return BitCast<uint64>(Value);
}

template<typename PointerType>
constexpr TEnableIf<TIsPointer<PointerType>::Value, uint64>::Type HashType(PointerType* Value) 
{
    return reinterpret_cast<uint64>(Value);
}

template<typename EnumType>
constexpr TEnableIf<TIsEnum<EnumType>::Value, uint64>::Type HashType(EnumType Value) 
{
    return static_cast<uint64>(ToUnderlying<EnumType>(Value));
}
