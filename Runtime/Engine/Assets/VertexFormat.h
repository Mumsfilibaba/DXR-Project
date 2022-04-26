#pragma once
#include "Core/Core.h"
#include "Core/Math/MathHash.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/FormatStructs.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVertex

struct SVertex
{
    SVertex()
        : Position()
        , Normal()
        , Tangent()
        , TexCoord()
    { }

    SVertex(const CVector3& InPosition, const CVector3& InNormal, const CVector3& InTangent, const CVector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TexCoord(InTexCoord)
    { }

    bool operator==(const SVertex& Other) const
    {
        return (Position == Other.Position) 
            && (Normal   == Other.Normal) 
            && (Tangent  == Other.Tangent) 
            && (TexCoord == Other.TexCoord);
    }

    bool operator!=(const SVertex& Other) const
    {
        return !(*this == Other);
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
        THash<CVector3> Hasher;

        size_t Hash = Hasher(Vertex.Position);
        HashCombine<CVector3>(Hash, Vertex.Normal);
        HashCombine<CVector3>(Hash, Vertex.Tangent);
        HashCombine<CVector2>(Hash, Vertex.TexCoord);
        return Hash;
    }
};