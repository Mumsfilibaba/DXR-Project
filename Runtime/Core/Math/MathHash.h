#pragma once
#include "Core/Math/Float.h"
#include "Core/Math/IntVector2.h"
#include "Core/Math/IntVector3.h"
#include "Core/Math/IntVector4.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Templates/TypeHash.h"

inline uint64 GetHashForType(FFloat16 Value)
{
    return GetHashForType(Value.Encoded);
}

inline uint64 GetHashForType(FFloat32 Value)
{
    return GetHashForType(Value.Encoded);
}

inline uint64 GetHashForType(FFloat64 Value)
{
    return GetHashForType(Value.Encoded);
}

inline uint64 GetHashForType(const Int16Vector2& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int16>(Hash, Value.Y);
    return Hash;
}

inline uint64 GetHashForType(const Int16Vector3& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int16>(Hash, Value.Y);
    HashCombine<int16>(Hash, Value.Z);
    return Hash;
}

inline uint64 GetHashForType(const Int16Vector4& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int16>(Hash, Value.Y);
    HashCombine<int16>(Hash, Value.Z);
    HashCombine<int16>(Hash, Value.W);
    return Hash;
}

inline uint64 GetHashForType(const IntVector2& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int32>(Hash, Value.Y);
    return Hash;
}

inline uint64 GetHashForType(const IntVector3& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int32>(Hash, Value.Y);
    HashCombine<int32>(Hash, Value.Z);
    return Hash;
}

inline uint64 GetHashForType(const IntVector4& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int32>(Hash, Value.Y);
    HashCombine<int32>(Hash, Value.Z);
    HashCombine<int32>(Hash, Value.W);
    return Hash;
}

inline uint64 GetHashForType(const Vector2& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<float>(Hash, Value.Y);
    return Hash;
}

inline uint64 GetHashForType(const Vector3& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<float>(Hash, Value.Y);
    HashCombine<float>(Hash, Value.Z);
    return Hash;
}

inline uint64 GetHashForType(const Vector4& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<float>(Hash, Value.Y);
    HashCombine<float>(Hash, Value.Z);
    HashCombine<float>(Hash, Value.W);
    return Hash;
}
