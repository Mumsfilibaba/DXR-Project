#pragma once
#include "RendererCore/Shaders/ShaderPermutation.h"
#include "RendererCore/VertexDeclaration.h"

class FParallax    : SHADER_PERMUTATION_BOOL(TEXT("ENABLE_PARALLAX_MAPPING"));
class FClipping    : SHADER_PERMUTATION_BOOL(TEXT("ENABLE_PARALLAX_CLIPPING"));
class FAlphaMask   : SHADER_PERMUTATION_BOOL(TEXT("ENABLE_ALPHA_MASK"));
class FDoubleSided : SHADER_PERMUTATION_BOOL(TEXT("ENABLE_DOUBLE_SIDED"));

using FMaterialPermutation = TShaderPermutation<FParallax, FClipping, FAlphaMask, FDoubleSided>;

template<typename PermutationType>
NODISCARD PermutationType RemapMaterialPermutation(PermutationType Permutation)
{
    if (!Permutation.template Get<FParallax>())
    {
        Permutation.template Set<FClipping>(false);
    }

    return Permutation;
}

class FRequiredAttributes : public TShaderPermutationInt<EVertexAttributeFlags, NUM_VERTEX_ATTRIBUTE_COMBINATIONS>
{
    SHADER_PERMUTATION_DEFINE(TEXT("VERTEX_ATTRIBUTES"));
};

NODISCARD inline EVertexAttributeFlags CreateDepthOnlyAttributes(bool bParallax, bool bAlphaMask)
{
    EVertexAttributeFlags Attributes = EVertexAttributeFlags::Position;
    if (bParallax || bAlphaMask)
    {
        Attributes |= EVertexAttributeFlags::TexCoord0;
    }

    if (bParallax)
    {
        Attributes |= EVertexAttributeFlags::TangentBasis;
    }

    return Attributes;
}

template<typename PermutationType>
NODISCARD PermutationType RemapDepthOnlyAttributes(PermutationType Permutation)
{
    Permutation.template Set<FRequiredAttributes>(
        CreateDepthOnlyAttributes(Permutation.template Get<FParallax>(), Permutation.template Get<FAlphaMask>()));
    return Permutation;
}

class FBindless : public FShaderPermutationBool
{
    SHADER_PERMUTATION_DEFINE(TEXT("ENABLE_BINDLESS"));

public:
    static void ModifyCompilationEnvironment(Type Value, FShaderCompilationEnvironment& Environment)
    {
        if (Value)
        {
            Environment.RequireShaderModel(EShaderModel::SM_6_6);
        }
    }
};

class FUnjitteredCamera : SHADER_PERMUTATION_BOOL(TEXT("USE_UNJITTERED_CAMERA"));
