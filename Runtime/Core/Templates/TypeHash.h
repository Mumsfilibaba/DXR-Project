#pragma once
#include "Core/Core.h"

#include <utility>
#include <functional>

template<typename T>
using THash = std::hash<T>;

template<typename T>
struct TTypeHash
{
    static uint64 Hash(const T& Object)
    {
        THash<T> Hasher;
        return Hasher(Object);
    }
};

template<>
struct TTypeHash<int8>
{
    static uint64 Hash(int8 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct TTypeHash<uint8>
{
    static uint64 Hash(uint8 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct TTypeHash<int16>
{
    static uint64 Hash(int16 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct TTypeHash<uint16>
{
    static uint64 Hash(uint16 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct TTypeHash<int32>
{
    static uint64 Hash(int32 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct TTypeHash<uint32>
{
    static uint64 Hash(uint32 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct TTypeHash<int64>
{
    static uint64 Hash(int64 Value)
    {
        return static_cast<uint64>(Value);
    }
};

template<>
struct TTypeHash<uint64>
{
    static uint64 Hash(uint64 Value)
    {
        return static_cast<uint64>(Value);
    }
};