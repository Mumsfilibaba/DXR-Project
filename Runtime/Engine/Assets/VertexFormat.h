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

    FSourceVertex(const FVector3& InPosition, const FVector3& InNormal, const FVector3& InTangent, const FVector2& InTexCoord)
        : Position(InPosition)
        , Normal(InNormal)
        , Tangent(InTangent)
        , TangentSign(1.0f)
        , TexCoord(InTexCoord)
    {
    }

    FSourceVertex(const FVector3& InPosition, const FVector3& InNormal, const FVector3& InTangent, float InTangentSign, const FVector2& InTexCoord)
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

    FVector3 Position;
    FVector3 Normal;
    FVector3 Tangent;
    float    TangentSign;
    FVector2 TexCoord;
};

static_assert(sizeof(FSourceVertex) == 48, "FSourceVertex must match the layout the .dxrmesh cache writes");

template<>
struct THash<FSourceVertex>
{
    static uint64 GetHash(const FSourceVertex& Vertex)
    {
        uint64 Result = THash<FVector3>::GetHash(Vertex.Position);
        HashCombine<FVector3>(Result, Vertex.Normal);
        HashCombine<FVector3>(Result, Vertex.Tangent);
        HashCombine<float>(Result, Vertex.TangentSign);
        HashCombine<FVector2>(Result, Vertex.TexCoord);
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

    FVertexAttributes(const FVector3& InNormal, const FVector3& InTangent, float InTangentSign, const FVector2& InTexCoord)
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
    FVector2     TexCoord;
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
        HashCombine<FVector2>(Result, Vertex.TexCoord);
        return Result;
    }
};
