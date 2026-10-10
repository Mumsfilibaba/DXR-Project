#pragma once
#include "Core/Math/Float.h"
#include "Core/Math/IntVector2.h"
#include "Core/Math/IntVector3.h"
#include "Core/Math/IntVector4.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Templates/TypeHash.h"

template<>
struct THash<FFloat16>
{
    static uint64 GetHash(FFloat16 Value)
    {
        return THash<decltype(Value.Encoded)>::GetHash(Value.Encoded);
    }
};

template<>
struct THash<FFloat32>
{
    static uint64 GetHash(FFloat32 Value)
    {
        return THash<decltype(Value.Encoded)>::GetHash(Value.Encoded);
    }
};

template<>
struct THash<FFloat64>
{
    static uint64 GetHash(FFloat64 Value)
    {
        return THash<decltype(Value.Encoded)>::GetHash(Value.Encoded);
    }
};

template<>
struct THash<FInt16Vector2>
{
    static uint64 GetHash(const FInt16Vector2& Value)
    {
        uint64 Result = THash<int16>::GetHash(Value.X);
        HashCombine<int16>(Result, Value.Y);
        return Result;
    }
};

template<>
struct THash<FInt16Vector3>
{
    static uint64 GetHash(const FInt16Vector3& Value)
    {
        uint64 Result = THash<int16>::GetHash(Value.X);
        HashCombine<int16>(Result, Value.Y);
        HashCombine<int16>(Result, Value.Z);
        return Result;
    }
};

template<>
struct THash<FInt16Vector4>
{
    static uint64 GetHash(const FInt16Vector4& Value)
    {
        uint64 Result = THash<int16>::GetHash(Value.X);
        HashCombine<int16>(Result, Value.Y);
        HashCombine<int16>(Result, Value.Z);
        HashCombine<int16>(Result, Value.W);
        return Result;
    }
};

template<>
struct THash<FIntVector2>
{
    static uint64 GetHash(const FIntVector2& Value)
    {
        uint64 Result = THash<int32>::GetHash(Value.X);
        HashCombine<int32>(Result, Value.Y);
        return Result;
    }
};

template<>
struct THash<FIntVector3>
{
    static uint64 GetHash(const FIntVector3& Value)
    {
        uint64 Result = THash<int32>::GetHash(Value.X);
        HashCombine<int32>(Result, Value.Y);
        HashCombine<int32>(Result, Value.Z);
        return Result;
    }
};

template<>
struct THash<FIntVector4>
{
    static uint64 GetHash(const FIntVector4& Value)
    {
        uint64 Result = THash<int32>::GetHash(Value.X);
        HashCombine<int32>(Result, Value.Y);
        HashCombine<int32>(Result, Value.Z);
        HashCombine<int32>(Result, Value.W);
        return Result;
    }
};

template<>
struct THash<FVector2>
{
    static uint64 GetHash(const FVector2& Value)
    {
        uint64 Result = THash<float>::GetHash(Value.X);
        HashCombine<float>(Result, Value.Y);
        return Result;
    }
};

template<>
struct THash<FVector3>
{
    static uint64 GetHash(const FVector3& Value)
    {
        uint64 Result = THash<float>::GetHash(Value.X);
        HashCombine<float>(Result, Value.Y);
        HashCombine<float>(Result, Value.Z);
        return Result;
    }
};

template<>
struct THash<FVector4>
{
    static uint64 GetHash(const FVector4& Value)
    {
        uint64 Result = THash<float>::GetHash(Value.X);
        HashCombine<float>(Result, Value.Y);
        HashCombine<float>(Result, Value.Z);
        HashCombine<float>(Result, Value.W);
        return Result;
    }
};
