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
        , TangentSign(1.0f)
        , TexCoord()
    {
    }

    FVertex(const Vector3& InPosition, const Vector3& InNormal, const Vector3& InTangent, const Vector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TangentSign(1.0f)
        , TexCoord(InTexCoord)
    {
    }

    FVertex(const Vector3& InPosition, const Vector3& InNormal, const Vector3& InTangent, float InTangentSign, const Vector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TangentSign(InTangentSign)
        , TexCoord(InTexCoord)
    {
    }

    bool operator==(const FVertex& Other) const
    {
        return Position == Other.Position && Normal == Other.Normal && Tangent == Other.Tangent && TangentSign == Other.TangentSign && TexCoord == Other.TexCoord;
    }

    bool operator!=(const FVertex& Other) const
    {
        return !(*this == Other);
    }

    Vector3 Position;
    Vector3 Normal;
    Vector3 Tangent;
    float   TangentSign; // MikkTSpace handedness: Bitangent = TangentSign * cross(Normal, Tangent)
    Vector2 TexCoord;
};

static_assert(sizeof(FVertex) == 48, "FVertex must match the HLSL FVertex layout");

template<>
struct THash<FVertex>
{
    static uint64 GetHash(const FVertex& Vertex)
    {
        uint64 Result = THash<Vector3>::GetHash(Vertex.Position);
        HashCombine<Vector3>(Result, Vertex.Normal);
        HashCombine<Vector3>(Result, Vertex.Tangent);
        HashCombine<float>(Result, Vertex.TangentSign);
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

static_assert(sizeof(FVertexPosition) == 12, "FVertexPosition must match the mesh input layout");

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
        , TangentSign(1.0f)
    {
    }

    FVertexNormal(const Vector3& InNormal, const Vector3& InTangent, float InTangentSign)
        : Normal(InNormal)
        , Tangent(InTangent)
        , TangentSign(InTangentSign)
    {
    }

    bool operator==(const FVertexNormal& Other) const
    {
        return Normal == Other.Normal && Tangent == Other.Tangent && TangentSign == Other.TangentSign;
    }

    bool operator!=(const FVertexNormal& Other) const
    {
        return !(*this == Other);
    }

    Vector3 Normal;
    Vector3 Tangent;
    float   TangentSign;
};

// The TANGENT input element reads xyz from Tangent and w from TangentSign as a single float4.
static_assert(sizeof(FVertexNormal) == 28, "FVertexNormal must match the mesh input layout");
static_assert(OFFSETOF(FVertexNormal, Tangent) == 12, "TANGENT input element expects Tangent at offset 12");

template<>
struct THash<FVertexNormal>
{
    static uint64 GetHash(const FVertexNormal& Vertex)
    {
        uint64 Result = THash<Vector3>::GetHash(Vertex.Normal);
        HashCombine<Vector3>(Result, Vertex.Tangent);
        HashCombine<float>(Result, Vertex.TangentSign);
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

static_assert(sizeof(FVertexTexCoord) == 8, "FVertexTexCoord must match the mesh input layout");

template<>
struct THash<FVertexTexCoord>
{
    static uint64 GetHash(const FVertexTexCoord& Vertex)
    {
        return THash<Vector2>::GetHash(Vertex.TexCoord);
    }
};
