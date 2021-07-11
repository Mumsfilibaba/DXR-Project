#pragma once
#include "Float.h"
#include "MathCommon.h"

// TODO: Remove these

// XMFLOAT2
inline XMFLOAT2 operator*( XMFLOAT2 LHS, float RHS )
{
    return XMFLOAT2( LHS.x * RHS, LHS.y * RHS );
}

inline XMFLOAT2 operator*( XMFLOAT2 LHS, XMFLOAT2 RHS )
{
    return XMFLOAT2( LHS.x * RHS.x, LHS.y * RHS.y );
}

inline XMFLOAT2 operator+( XMFLOAT2 LHS, XMFLOAT2 RHS )
{
    return XMFLOAT2( LHS.x + RHS.x, LHS.y + RHS.y );
}

inline XMFLOAT2 operator-( XMFLOAT2 LHS, XMFLOAT2 RHS )
{
    return XMFLOAT2( LHS.x - RHS.x, LHS.y - RHS.y );
}

inline XMFLOAT2 operator-( XMFLOAT2 Value )
{
    return XMFLOAT2( -Value.x, -Value.y );
}

inline float Length( const XMFLOAT2& Vector )
{
    return (float)sqrt( Vector.x * Vector.x + Vector.y * Vector.y );
}

// XMFLOAT3
inline XMFLOAT3 operator*( XMFLOAT3 LHS, float RHS )
{
    return XMFLOAT3( LHS.x * RHS, LHS.y * RHS, LHS.z * RHS );
}

inline XMFLOAT3 operator*( XMFLOAT3 LHS, XMFLOAT3 RHS )
{
    return XMFLOAT3( LHS.x * RHS.x, LHS.y * RHS.y, LHS.z * RHS.z );
}

inline XMFLOAT3 operator/( XMFLOAT3 LHS, float RHS )
{
    return XMFLOAT3( LHS.x / RHS, LHS.y / RHS, LHS.z / RHS );
}

inline XMFLOAT3 operator/( XMFLOAT3 LHS, XMFLOAT3 RHS )
{
    return XMFLOAT3( LHS.x / RHS.x, LHS.y / RHS.y, LHS.z / RHS.z );
}

inline XMFLOAT3 operator+( XMFLOAT3 LHS, XMFLOAT3 RHS )
{
    return XMFLOAT3( LHS.x + RHS.x, LHS.y + RHS.y, LHS.z + RHS.z );
}

inline XMFLOAT3 operator-( XMFLOAT3 LHS, XMFLOAT3 RHS )
{
    return XMFLOAT3( LHS.x - RHS.x, LHS.y - RHS.y, LHS.z - RHS.z );
}

inline XMFLOAT3 operator-( XMFLOAT3 Value )
{
    return XMFLOAT3( -Value.x, -Value.y, -Value.z );
}

inline float Length( const XMFLOAT3& Vector )
{
    return (float)sqrt( Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z );
}

// XMFLOAT4
inline XMFLOAT4 operator*( XMFLOAT4 LHS, float RHS )
{
    return XMFLOAT4( LHS.x * RHS, LHS.y * RHS, LHS.z * RHS, LHS.w * RHS );
}

inline XMFLOAT4 operator*( XMFLOAT4 LHS, XMFLOAT4 RHS )
{
    return XMFLOAT4( LHS.x * RHS.x, LHS.y * RHS.y, LHS.z * RHS.z, LHS.w * RHS.w );
}

inline XMFLOAT4 operator/( XMFLOAT4 LHS, float RHS )
{
    return XMFLOAT4( LHS.x / RHS, LHS.y / RHS, LHS.z / RHS, LHS.w / RHS );
}

inline XMFLOAT4 operator/( XMFLOAT4 LHS, XMFLOAT4 RHS )
{
    return XMFLOAT4( LHS.x / RHS.x, LHS.y / RHS.y, LHS.z / RHS.z, LHS.w / RHS.w );
}

inline XMFLOAT4 operator+( XMFLOAT4 LHS, XMFLOAT4 RHS )
{
    return XMFLOAT4( LHS.x + RHS.x, LHS.y + RHS.y, LHS.z + RHS.z, LHS.w + RHS.w );
}

inline XMFLOAT4 operator-( XMFLOAT4 LHS, XMFLOAT4 RHS )
{
    return XMFLOAT4( LHS.x - RHS.x, LHS.y - RHS.y, LHS.z - RHS.z, LHS.w - RHS.w );
}

inline XMFLOAT4 operator-( XMFLOAT4 Value )
{
    return XMFLOAT4( -Value.x, -Value.y, -Value.z, -Value.w );
}

inline float Length( const XMFLOAT4& Vector )
{
    return (float)sqrt( Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z + Vector.w * Vector.w );
}