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

    FVertex(const Vector3& InPosition, const Vector3& InNormal, const Vector3& InTangent, const Vector2& InTexCoord)
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
        HashCombine<Vector3>(Hash, Vertex.Normal);
        HashCombine<Vector3>(Hash, Vertex.Tangent);
        HashCombine<Vector2>(Hash, Vertex.TexCoord);
        return Hash;
    }

    Vector3 Position;
    Vector3 Normal;
    Vector3 Tangent;
    Vector2 TexCoord;
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

    FVertexPacked(const Vector3& InPosition, const FR10G10B10A2& InNormal, const FR10G10B10A2& InTangent, const Vector2& InTexCoord)
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

    Vector3      Position;
    FR10G10B10A2 Normal;
    FR10G10B10A2 Tangent;
    Vector2      TexCoord;
};

struct FVertexPosition
{
    FVertexPosition()
        : Position()
    {
    }

    FVertexPosition(const Vector3& InPosition)
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

    Vector3 Position;
};

struct FVertexNormal
{
    FVertexNormal()
        : Normal()
        , Tangent()
    {
    }

    FVertexNormal(const Vector3& InNormal, const Vector3& InTangent)
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
        HashCombine<Vector3>(Hash, Vertex.Tangent);
        return Hash;
    }

    Vector3 Normal;
    Vector3 Tangent;
};

struct FVertexTexCoord
{
    FVertexTexCoord()
        : TexCoord()
    {
    }

    FVertexTexCoord(const Vector2& InTexCoord)
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

    Vector2 TexCoord;
};
