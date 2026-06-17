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

    Vector3 Position;
    Vector3 Normal;
    Vector3 Tangent;
    Vector2 TexCoord;
};

template<>
struct THash<FVertex>
{
    static uint64 GetHash(const FVertex& Vertex)
    {
        uint64 Result = THash<Vector3>::GetHash(Vertex.Position);
        HashCombine<Vector3>(Result, Vertex.Normal);
        HashCombine<Vector3>(Result, Vertex.Tangent);
        HashCombine<Vector2>(Result, Vertex.TexCoord);
        return Result;
    }
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

    Vector3 Position;
};

template<>
struct THash<FVertexPosition>
{
    static uint64 GetHash(const FVertexPosition& Vertex)
    {
        return THash<Vector3>::GetHash(Vertex.Position);
    }
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

    Vector3 Normal;
    Vector3 Tangent;
};

template<>
struct THash<FVertexNormal>
{
    static uint64 GetHash(const FVertexNormal& Vertex)
    {
        uint64 Result = THash<Vector3>::GetHash(Vertex.Normal);
        HashCombine<Vector3>(Result, Vertex.Tangent);
        return Result;
    }
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

    Vector2 TexCoord;
};

template<>
struct THash<FVertexTexCoord>
{
    static uint64 GetHash(const FVertexTexCoord& Vertex)
    {
        return THash<Vector2>::GetHash(Vertex.TexCoord);
    }
};
