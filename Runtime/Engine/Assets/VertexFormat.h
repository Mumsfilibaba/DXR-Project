#pragma once
#include "Core/Core.h"
#include "Core/Math/MathHash.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/FormatStructs.h"

struct FSourceVertex
{
    FSourceVertex()
        : Position()
        , Normal()
        , Tangent()
        , TangentSign(1.0f)
        , TexCoord()
    {
    }

    FSourceVertex(const Vector3& InPosition, const Vector3& InNormal, const Vector3& InTangent, const Vector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TangentSign(1.0f)
        , TexCoord(InTexCoord)
    {
    }

    FSourceVertex(const Vector3& InPosition, const Vector3& InNormal, const Vector3& InTangent, float InTangentSign, const Vector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TangentSign(InTangentSign)
        , TexCoord(InTexCoord)
    {
    }

    bool operator==(const FSourceVertex& Other) const
    {
        return Position == Other.Position && Normal == Other.Normal && Tangent == Other.Tangent && TangentSign == Other.TangentSign && TexCoord == Other.TexCoord;
    }

    bool operator!=(const FSourceVertex& Other) const
    {
        return !(*this == Other);
    }

    Vector3 Position;
    Vector3 Normal;
    Vector3 Tangent;
    float   TangentSign;
    Vector2 TexCoord;
};

static_assert(sizeof(FSourceVertex) == 48, "FSourceVertex must match the layout the .dxrmesh cache writes");

template<>
struct THash<FSourceVertex>
{
    static uint64 GetHash(const FSourceVertex& Vertex)
    {
        uint64 Result = THash<Vector3>::GetHash(Vertex.Position);
        HashCombine<Vector3>(Result, Vertex.Normal);
        HashCombine<Vector3>(Result, Vertex.Tangent);
        HashCombine<float>(Result, Vertex.TangentSign);
        HashCombine<Vector2>(Result, Vertex.TexCoord);
        return Result;
    }
};

struct FVertexAttributes
{
    FVertexAttributes()
        : Normal()
        , Tangent()
        , TexCoord()
    {
    }

    FVertexAttributes(const Vector3& InNormal, const Vector3& InTangent, float InTangentSign, const Vector2& InTexCoord)
        : Normal(InNormal)
        , Tangent(InTangent, InTangentSign)
        , TexCoord(InTexCoord)
    {
    }

    bool operator==(const FVertexAttributes& Other) const
    {
        return Normal == Other.Normal && Tangent == Other.Tangent && TexCoord == Other.TexCoord;
    }

    bool operator!=(const FVertexAttributes& Other) const
    {
        return !(*this == Other);
    }

    FRGBA16Snorm Normal;   // w unused
    FRGBA16Snorm Tangent;  // w = handedness sign
    Vector2      TexCoord;
};

static_assert(sizeof(FVertexAttributes) == 24, "FVertexAttributes must match the attribute stream stride");
static_assert(OFFSETOF(FVertexAttributes, Tangent) == 8, "The TANGENT input element expects Tangent at offset 8");
static_assert(OFFSETOF(FVertexAttributes, TexCoord) == 16, "The TEXCOORD input element expects TexCoord at offset 16");

template<>
struct THash<FVertexAttributes>
{
    static uint64 GetHash(const FVertexAttributes& Vertex)
    {
        uint64 Result = THash<FRGBA16Snorm>::GetHash(Vertex.Normal);
        HashCombine<FRGBA16Snorm>(Result, Vertex.Tangent);
        HashCombine<Vector2>(Result, Vertex.TexCoord);
        return Result;
    }
};
