#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

#include "Utilities/HashUtilities.h"

namespace std
{
template<> 
struct hash<CVector4>
{
    size_t operator()( const CVector4& v ) const
    {
        std::hash<float> Hasher;

        size_t Hash = Hasher( v.x );
        HashCombine<float>( Hash, v.y );
        HashCombine<float>( Hash, v.z );
        HashCombine<float>( Hash, v.w );
        return Hash;
    }
};

template<> 
struct hash<CVector3>
{
    size_t operator()( const CVector3& v ) const
    {
        std::hash<float> Hasher;

        size_t Hash = Hasher( v.x );
        HashCombine<float>( Hash, v.y );
        HashCombine<float>( Hash, v.z );
        return Hash;
    }
};

template<> 
struct hash<CVector2>
{
    size_t operator()( const CVector2& v ) const
    {
        std::hash<float> Hasher;

        size_t Hash = Hasher( v.x );
        HashCombine<float>( Hash, v.y );
        return Hash;
    }
};
}