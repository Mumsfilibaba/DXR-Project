#pragma once
#include "Core/Core.h"

#include "Core/Math/MathHash.h"

struct SVertex
{
    CVector3 Position;
    CVector3 Normal;
    CVector3 Tangent;
    CVector2 TexCoord;

    FORCEINLINE bool operator==(const SVertex& Other) const
    {
        return (Position == Other.Position) && (Normal == Other.Normal) && (Tangent == Other.Tangent) && (TexCoord == Other.TexCoord);
    }

    FORCEINLINE bool operator!=(const SVertex& Other) const
    {
        return !(*this == Other);
    }
};

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