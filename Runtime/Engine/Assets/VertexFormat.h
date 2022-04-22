#pragma once
#include "Core/Core.h"

#include "Core/Math/MathHash.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVertex

struct SVertex
{
    FORCEINLINE bool operator==(const SVertex& RHS) const
    {
        return (Position == RHS.Position) 
            && (Normal   == RHS.Normal) 
            && (Tangent  == RHS.Tangent) 
            && (TexCoord == RHS.TexCoord);
    }

    FORCEINLINE bool operator!=(const SVertex& RHS) const
    {
        return !(*this == RHS);
    }

    CVector3 Position;
    CVector3 Normal;
    CVector3 Tangent;
    CVector2 TexCoord;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVertexHasher

struct SVertexHasher
{
    inline size_t operator()(const SVertex& Vertex) const
    {
        std::hash<CVector3> Hasher;

        size_t Hash = Hasher(Vertex.Position);
        HashCombine<CVector3>(Hash, Vertex.Normal);
        HashCombine<CVector3>(Hash, Vertex.Tangent);
        HashCombine<CVector2>(Hash, Vertex.TexCoord);
        return Hash;
    }
};