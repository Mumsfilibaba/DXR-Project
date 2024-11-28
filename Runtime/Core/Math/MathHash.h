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

inline uint64 GetHashForType(const FInt16Vector2& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int16>(Hash, Value.Y);
    return Hash;
}

inline uint64 GetHashForType(const FInt16Vector3& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int16>(Hash, Value.Y);
    HashCombine<int16>(Hash, Value.Z);
    return Hash;
}

inline uint64 GetHashForType(const FInt16Vector4& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int16>(Hash, Value.Y);
    HashCombine<int16>(Hash, Value.Z);
    HashCombine<int16>(Hash, Value.W);
    return Hash;
}

inline uint64 GetHashForType(const FIntVector2& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int32>(Hash, Value.Y);
    return Hash;
}

inline uint64 GetHashForType(const FIntVector3& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int32>(Hash, Value.Y);
    HashCombine<int32>(Hash, Value.Z);
    return Hash;
}

inline uint64 GetHashForType(const FIntVector4& Value)
{
    uint64 Hash = GetHashForType(Value.X);
    HashCombine<int32>(Hash, Value.Y);
    HashCombine<int32>(Hash, Value.Z);
    HashCombine<int32>(Hash, Value.W);
    return Hash;
}

inline uint64 GetHashForType(const FVector2& Value)
{
    uint64 Hash = GetHashForType(Value.x);
    HashCombine<float>(Hash, Value.y);
    return Hash;
}

inline uint64 GetHashForType(const FVector3& Value)
{
    uint64 Hash = GetHashForType(Value.x);
    HashCombine<float>(Hash, Value.y);
    HashCombine<float>(Hash, Value.z);
    return Hash;
}

inline uint64 GetHashForType(const FVector4& Value)
{
    uint64 Hash = GetHashForType(Value.x);
    HashCombine<float>(Hash, Value.y);
    HashCombine<float>(Hash, Value.z);
    HashCombine<float>(Hash, Value.w);
    return Hash;
}
