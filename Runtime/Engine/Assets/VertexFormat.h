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

    SVertex(const FVector3& InPosition, const FVector3& InNormal, const FVector3& InTangent, const FVector2& InTexCoord)
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

    FVector3 Position;
    FVector3 Normal;
    FVector3 Tangent;
    FVector2 TexCoord;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVertexHasher

struct SVertexHasher
{
    inline size_t operator()(const SVertex& Vertex) const
    {
        THash<FVector3> Hasher;

        size_t Hash = Hasher(Vertex.Position);
        HashCombine<FVector3>(Hash, Vertex.Normal);
        HashCombine<FVector3>(Hash, Vertex.Tangent);
        HashCombine<FVector2>(Hash, Vertex.TexCoord);
        return Hash;
    }
};