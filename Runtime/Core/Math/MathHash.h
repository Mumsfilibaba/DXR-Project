#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Float.h"
#include "Core/Utilities/HashUtilities.h"

namespace std
{
    template<>
    struct hash<FVector4>
    {
        size_t operator()(const FVector4& Value) const
        {
            uint64 Hash = TTypeHash<float>::Hash(Value.x);
            HashCombine<float>(Hash, Value.y);
            HashCombine<float>(Hash, Value.z);
            HashCombine<float>(Hash, Value.w);
            return Hash;
        }
    };

    template<>
    struct hash<FVector3>
    {
        size_t operator()(const FVector3& Value) const
        {
            uint64 Hash = TTypeHash<float>::Hash(Value.x);
            HashCombine<float>(Hash, Value.y);
            HashCombine<float>(Hash, Value.z);
            return Hash;
        }
    };

    template<>
    struct hash<FVector2>
    {
        size_t operator()(const FVector2& Value) const
        {
            uint64 Hash = TTypeHash<float>::Hash(Value.x);
            HashCombine<float>(Hash, Value.y);
            return Hash;
        }
    };

    
    template<>
    struct hash<FFloat16>
    {
        size_t operator()(const FFloat16& Value) const
        {
            return TTypeHash<uint16>::Hash(Value.Encoded);
        }
    };

    template<>
    struct hash<FFloat32>
    {
        size_t operator()(const FFloat32& Value) const
        {
            return TTypeHash<uint32>::Hash(Value.Encoded);
        }
    };

    template<>
    struct hash<FFloat64>
    {
        size_t operator()(const FFloat64& Value) const
        {
            return TTypeHash<uint64>::Hash(Value.Encoded);
        }
    };
}
