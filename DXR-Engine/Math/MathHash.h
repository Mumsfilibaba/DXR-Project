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

// TODO: Remove these at some point

template<> 
struct hash<XMFLOAT4>
{
    size_t operator()( const XMFLOAT4& XmFloat ) const
    {
        std::hash<float> Hasher;

        size_t Hash = Hasher( XmFloat.x );
        HashCombine<float>( Hash, XmFloat.y );
        HashCombine<float>( Hash, XmFloat.z );
        HashCombine<float>( Hash, XmFloat.w );

        return Hash;
    }
};

template<> 
struct hash<XMFLOAT3>
{
    size_t operator()( const XMFLOAT3& XmFloat ) const
    {
        std::hash<float> Hasher;

        size_t Hash = Hasher( XmFloat.x );
        HashCombine<float>( Hash, XmFloat.y );
        HashCombine<float>( Hash, XmFloat.z );

        return Hash;
    }
};

template<> 
struct hash<XMFLOAT2>
{
    size_t operator()( const XMFLOAT2& XmFloat ) const
    {
        std::hash<float> Hasher;

        size_t Hash = Hasher( XmFloat.x );
        HashCombine<float>( Hash, XmFloat.y );

        return Hash;
    }
};
}