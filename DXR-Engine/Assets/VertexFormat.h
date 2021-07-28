#pragma once
#include "Core.h"

#include "Core/Math/MathHash.h"

struct Vertex
{
    CVector3 Position;
    CVector3 Normal;
    CVector3 Tangent;
    CVector2 TexCoord;

    FORCEINLINE bool operator==( const Vertex& Other ) const
    {
        return Position == (Other.Position) && (Normal == Other.Normal) && (Tangent == Other.Tangent) && (TexCoord == Other.TexCoord);
    }

    FORCEINLINE bool operator!=( const Vertex& Other ) const
    {
        return !(*this == Other);
    }
};

struct VertexHasher
{
    inline size_t operator()( const Vertex& v ) const
    {
        std::hash<CVector3> Hasher;

        size_t Hash = Hasher( v.Position );
        HashCombine<CVector3>( Hash, v.Normal );
        HashCombine<CVector3>( Hash, v.Tangent );
        HashCombine<CVector2>( Hash, v.TexCoord );
        return Hash;
    }
};