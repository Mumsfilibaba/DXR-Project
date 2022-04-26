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
    struct hash<CVector4>
    {
        size_t operator()(const CVector4& Value) const
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
    struct hash<CVector3>
    {
        size_t operator()(const CVector3& Value) const
        {
            THash<float> Hasher;

            size_t Hash = Hasher(Value.x);
            HashCombine<float>(Hash, Value.y);
            HashCombine<float>(Hash, Value.z);
            return Hash;
        }
    };

    template<>
    struct hash<CVector2>
    {
        size_t operator()(const CVector2& Value) const
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
    struct hash<SFloat16>
    {
        size_t operator()(const SFloat16& Value) const
        {
            THash<uint16> Hasher;
            return Hasher(Value.Encoded);
        }
    };

    template<>
    struct hash<SFloat32>
    {
        size_t operator()(const SFloat32& Value) const
        {
            THash<uint32> Hasher;
            return Hasher(Value.Encoded);
        }
    };

    template<>
    struct hash<SFloat64>
    {
        size_t operator()(const SFloat64& Value) const
        {
            THash<uint64> Hasher;
            return Hasher(Value.Encoded);
        }
    };
}