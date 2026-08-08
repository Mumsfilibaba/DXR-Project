#pragma once
#include "Renderer/RenderPass.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/Shaders/ShaderType.h"

struct FPrePassShaderRules
{
    using FPermutation = TShaderPermutation<FMaterialPermutation, FBindless, FUnjitteredCamera, FRequiredAttributes>;

    static_assert(FPermutation::PermutationCount == 1024, "PrePass permutation space grew unexpectedly");

    NODISCARD static FPermutation Create(const FMaterialFeatures& Features, bool bBindless, bool bUnjitteredCamera)
    {
        FPermutation Permutation;
        Permutation.Set<FMaterialPermutation>(Features.CreatePermutation());
        Permutation.Set<FBindless>(bBindless);
        Permutation.Set<FUnjitteredCamera>(bUnjitteredCamera);
        return RemapPermutation(Permutation);
    }

    NODISCARD static FPermutation RemapPermutation(FPermutation Permutation)
    {
        return RemapDepthOnlyAttributes(RemapMaterialPermutation(Permutation));
    }

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        if (Permutation.Get<FBindless>() && !Desc.bSupportsBindless)
        {
            return false;
        }

        return RemapPermutation(Permutation) == Permutation;
    }
};

class FPrePassVS : public FPrePassShaderRules
{
    DECLARE_SHADER_TYPE(FPrePassVS, EShaderStage::Vertex);
};

class FPrePassPS : public FPrePassShaderRules
{
    DECLARE_SHADER_TYPE(FPrePassPS, EShaderStage::Pixel);
};
