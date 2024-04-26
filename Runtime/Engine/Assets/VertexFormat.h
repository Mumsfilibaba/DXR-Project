#pragma once
#include "Core/Core.h"
#include "Core/Math/MathHash.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/FormatStructs.h"

struct FVertex
{
    FVertex()
        : Position()
        , Normal()
        , Tangent()
        , TexCoord()
    {
    }

    FVertex(const FVector3& InPosition, const FVector3& InNormal, const FVector3& InTangent, const FVector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TexCoord(InTexCoord)
    {
    }

    bool operator==(const FVertex& Other) const
    {
        return Position == Other.Position && Normal == Other.Normal && Tangent == Other.Tangent && TexCoord == Other.TexCoord;
    }

    bool operator!=(const FVertex& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVertex& Vertex)
    {
        uint64 Hash = GetHashForType(Vertex.Position);
        HashCombine<FVector3>(Hash, Vertex.Normal);
        HashCombine<FVector3>(Hash, Vertex.Tangent);
        HashCombine<FVector2>(Hash, Vertex.TexCoord);
        return Hash;
    }

    FVector3 Position;
    FVector3 Normal;
    FVector3 Tangent;
    FVector2 TexCoord;
};

struct FVertexMasked
{
    FVertexMasked()
        : Position()
        , TexCoord()
    {
    }

    FVertexMasked(const FVector3& InPosition, const FVector2& InTexCoord)
        : Position(InPosition)
        , TexCoord(InTexCoord)
    {
    }

    bool operator==(const FVertexMasked& Other) const
    {
        return Position == Other.Position && TexCoord == Other.TexCoord;
    }

    bool operator!=(const FVertexMasked& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVertexMasked& Vertex)
    {
        uint64 Hash = GetHashForType(Vertex.Position);
        HashCombine<FVector2>(Hash, Vertex.TexCoord);
        return Hash;
    }

    FVector3 Position;
    FVector2 TexCoord;
};

struct FVertexPacked
{
    FVertexPacked()
        : Position()
        , Normal()
        , Tangent()
        , TexCoord()
    {
    }

    FVertexPacked(const FVector3& InPosition, const FR10G10B10A2& InNormal, const FR10G10B10A2& InTangent, const FVector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TexCoord(InTexCoord)
    {
    }

    bool operator==(const FVertexPacked& Other) const
    {
        return Position == Other.Position && Normal == Other.Normal && Tangent == Other.Tangent && TexCoord == Other.TexCoord;
    }

    bool operator!=(const FVertexPacked& Other) const
    {
        return !(*this == Other);
    }

    FVector3     Position;
    FR10G10B10A2 Normal;
    FR10G10B10A2 Tangent;
    FVector2     TexCoord;
};

