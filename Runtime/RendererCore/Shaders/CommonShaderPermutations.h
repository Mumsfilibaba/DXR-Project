#pragma once
#include "RendererCore/Shaders/ShaderPermutation.h"
#include "RendererCore/VertexDeclaration.h"

class FParallax    : SHADER_PERMUTATION_BOOL("ENABLE_PARALLAX_MAPPING");
class FClipping    : SHADER_PERMUTATION_BOOL("ENABLE_PARALLAX_CLIPPING");
class FAlphaMask   : SHADER_PERMUTATION_BOOL("ENABLE_ALPHA_MASK");
class FDoubleSided : SHADER_PERMUTATION_BOOL("ENABLE_DOUBLE_SIDED");

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
    SHADER_PERMUTATION_DEFINE("VERTEX_ATTRIBUTES");
};

NODISCARD inline EVertexAttributeFlags MakeDepthOnlyAttributes(bool bParallax, bool bAlphaMask)
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
        MakeDepthOnlyAttributes(Permutation.template Get<FParallax>(), Permutation.template Get<FAlphaMask>()));
    return Permutation;
}

class FBindless : public FShaderPermutationBool
{
    SHADER_PERMUTATION_DEFINE("ENABLE_BINDLESS");

public:
    static void ModifyCompilationEnvironment(Type Value, FShaderCompilationEnvironment& Environment)
    {
        if (Value)
        {
            Environment.RequireShaderModel(EShaderModel::SM_6_6);
        }
    }
};

class FUnjitteredCamera : SHADER_PERMUTATION_BOOL("USE_UNJITTERED_CAMERA");
