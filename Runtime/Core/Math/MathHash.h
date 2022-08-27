#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Float.h"

#include "Core/Utilities/HashUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// THash

namespace std
{
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Vectors

    template<>
    struct hash<FVector4>
    {
        size_t operator()(const FVector4& Value) const
        {
            THash<float> Hasher;

            size_t Hash = Hasher(Value.x);
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
            THash<float> Hasher;

            size_t Hash = Hasher(Value.x);
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
            THash<float> Hasher;

            size_t Hash = Hasher(Value.x);
            HashCombine<float>(Hash, Value.y);
            return Hash;
        }
    };
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Floats
    
    template<>
    struct hash<FFloat16>
    {
        size_t operator()(const FFloat16& Value) const
        {
            THash<uint16> Hasher;
            return Hasher(Value.Encoded);
        }
    };

    template<>
    struct hash<FFloat32>
    {
        size_t operator()(const FFloat32& Value) const
        {
            THash<uint32> Hasher;
            return Hasher(Value.Encoded);
        }
    };

    template<>
    struct hash<FFloat64>
    {
        size_t operator()(const FFloat64& Value) const
        {
            THash<uint64> Hasher;
            return Hasher(Value.Encoded);
        }
    };
}