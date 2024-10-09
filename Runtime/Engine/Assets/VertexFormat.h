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

struct FVertexPosition
{
    FVertexPosition()
        : Position()
    {
    }

    FVertexPosition(const FVector3& InPosition)
        : Position(InPosition)
    {
    }

    bool operator==(const FVertexPosition& Other) const
    {
        return Position == Other.Position;
    }

    bool operator!=(const FVertexPosition& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVertexPosition& Vertex)
    {
        uint64 Hash = GetHashForType(Vertex.Position);
        return Hash;
    }

    FVector3 Position;
};

struct FVertexNormal
{
    FVertexNormal()
        : Normal()
        , Tangent()
    {
    }

    FVertexNormal(const FVector3& InNormal, const FVector3& InTangent)
        : Normal(InNormal)
        , Tangent(InTangent)
    {
    }

    bool operator==(const FVertexNormal& Other) const
    {
        return Normal == Other.Normal && Tangent == Other.Tangent;
    }

    bool operator!=(const FVertexNormal& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVertexNormal& Vertex)
    {
        uint64 Hash = GetHashForType(Vertex.Normal);
        HashCombine<FVector3>(Hash, Vertex.Tangent);
        return Hash;
    }

    FVector3 Normal;
    FVector3 Tangent;
};

struct FVertexTexCoord
{
    FVertexTexCoord()
        : TexCoord()
    {
    }

    FVertexTexCoord(const FVector2& InTexCoord)
        : TexCoord(InTexCoord)
    {
    }

    bool operator==(const FVertexTexCoord& Other) const
    {
        return TexCoord == Other.TexCoord;
    }

    bool operator!=(const FVertexTexCoord& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVertexTexCoord& Vertex)
    {
        uint64 Hash = GetHashForType(Vertex.TexCoord);
        return Hash;
    }

    FVector2 TexCoord;
};
