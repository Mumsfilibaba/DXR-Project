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
struct THash<Int16Vector2>
{
    static uint64 GetHash(const Int16Vector2& Value)
    {
        uint64 Result = THash<int16>::GetHash(Value.X);
        HashCombine<int16>(Result, Value.Y);
        return Result;
    }
};

template<>
struct THash<Int16Vector3>
{
    static uint64 GetHash(const Int16Vector3& Value)
    {
        uint64 Result = THash<int16>::GetHash(Value.X);
        HashCombine<int16>(Result, Value.Y);
        HashCombine<int16>(Result, Value.Z);
        return Result;
    }
};

template<>
struct THash<Int16Vector4>
{
    static uint64 GetHash(const Int16Vector4& Value)
    {
        uint64 Result = THash<int16>::GetHash(Value.X);
        HashCombine<int16>(Result, Value.Y);
        HashCombine<int16>(Result, Value.Z);
        HashCombine<int16>(Result, Value.W);
        return Result;
    }
};

template<>
struct THash<IntVector2>
{
    static uint64 GetHash(const IntVector2& Value)
    {
        uint64 Result = THash<int32>::GetHash(Value.X);
        HashCombine<int32>(Result, Value.Y);
        return Result;
    }
};

template<>
struct THash<IntVector3>
{
    static uint64 GetHash(const IntVector3& Value)
    {
        uint64 Result = THash<int32>::GetHash(Value.X);
        HashCombine<int32>(Result, Value.Y);
        HashCombine<int32>(Result, Value.Z);
        return Result;
    }
};

template<>
struct THash<IntVector4>
{
    static uint64 GetHash(const IntVector4& Value)
    {
        uint64 Result = THash<int32>::GetHash(Value.X);
        HashCombine<int32>(Result, Value.Y);
        HashCombine<int32>(Result, Value.Z);
        HashCombine<int32>(Result, Value.W);
        return Result;
    }
};

template<>
struct THash<Vector2>
{
    static uint64 GetHash(const Vector2& Value)
    {
        uint64 Result = THash<float>::GetHash(Value.X);
        HashCombine<float>(Result, Value.Y);
        return Result;
    }
};

template<>
struct THash<Vector3>
{
    static uint64 GetHash(const Vector3& Value)
    {
        uint64 Result = THash<float>::GetHash(Value.X);
        HashCombine<float>(Result, Value.Y);
        HashCombine<float>(Result, Value.Z);
        return Result;
    }
};

template<>
struct THash<Vector4>
{
    static uint64 GetHash(const Vector4& Value)
    {
        uint64 Result = THash<float>::GetHash(Value.X);
        HashCombine<float>(Result, Value.Y);
        HashCombine<float>(Result, Value.Z);
        HashCombine<float>(Result, Value.W);
        return Result;
    }
};
