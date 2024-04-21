#pragma once
#include "BitCast.h"
#include "Core/Core.h"

constexpr uint64 GetHashForType(bool bValue)
{
    return static_cast<uint64>(bValue);
}

constexpr uint64 GetHashForType(int8 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint8 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(int16 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint16 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(int32 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint32 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(int64 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(uint64 Value)
{
    return static_cast<uint64>(Value);
}

constexpr uint64 GetHashForType(float Value)
{
    return static_cast<uint64>(BitCast<uint32>(Value));
}

constexpr uint64 GetHashForType(double Value)
{
    return BitCast<uint64>(Value);
}

template<typename PointerType>
constexpr TEnableIf<TIsPointer<PointerType>::Value, uint64>::Type GetHashForType(PointerType* Value) 
{
    return reinterpret_cast<uint64>(Value);
}

template<typename EnumType>
constexpr TEnableIf<TIsEnum<EnumType>::Value, uint64>::Type GetHashForType(EnumType Value) 
{
    return static_cast<uint64>(ToUnderlying<EnumType>(Value));
}
