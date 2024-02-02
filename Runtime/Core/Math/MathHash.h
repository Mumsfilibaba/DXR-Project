#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Float.h"
#include "Core/Utilities/HashUtilities.h"

inline uint64 HashType(FFloat16 Value)
{
    return HashType(Value.Encoded);
}

inline uint64 HashType(FFloat32 Value)
{
    return HashType(Value.Encoded);
}

inline uint64 HashType(FFloat64 Value)
{
    return HashType(Value.Encoded);
}

inline uint64 HashType(const FVector2& Value)
{
    uint64 Hash = HashType(Value.x);
    HashCombine<float>(Hash, Value.y);
    return Hash;
}

inline uint64 HashType(const FVector3& Value)
{
    uint64 Hash = HashType(Value.x);
    HashCombine<float>(Hash, Value.y);
    HashCombine<float>(Hash, Value.z);
    return Hash;
}

inline uint64 HashType(const FVector4& Value)
{
    uint64 Hash = HashType(Value.x);
    HashCombine<float>(Hash, Value.y);
    HashCombine<float>(Hash, Value.z);
    HashCombine<float>(Hash, Value.w);
    return Hash;
}