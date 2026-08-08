#pragma once
#include "Renderer/RenderPass.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/Shaders/ShaderType.h"

enum class ECubeMapRenderPassType : uint8
{
    Unknown = 0,
    MultiPass,
    SinglePass,
    GeometryShaderSinglePass,

    First = MultiPass,
    Last = GeometryShaderSinglePass,

    Count = Last + 1,
};

enum class ECascadeRenderPassType : uint8
{
    Unknown = 0,
    MultiPass,
    SinglePass,
    GeometryShaderSinglePass,
    ViewInstancingSinglePass,

    First = MultiPass,
    Last = ViewInstancingSinglePass,

    Count = Last + 1,
};

enum class ECSMFilterMode : uint8
{
    PCF  = 0,
    PCSS = 1,

    Count,
};

enum class ECSMFilterFunction : uint8
{
    Grid        = 0,
    PoissonDisk = 1,
    VogelDisk   = 2,

    Count,
};

template<typename ShaderRules, typename PassKindDimension>
NODISCARD typename ShaderRules::FPermutation MakeShadowPermutation(const FMaterialFeatures& Features, bool bBindless, typename PassKindDimension::Type PassKind)
{
    typename ShaderRules::FPermutation Permutation;
    Permutation.template Set<FParallax>(Features.HasHeightMap());
    Permutation.template Set<FClipping>(Features.HasParallaxClipping());
    Permutation.template Set<FAlphaMask>(Features.HasAlphaMask());
    Permutation.template Set<FBindless>(bBindless);
    Permutation.template Set<PassKindDimension>(PassKind);
    return ShaderRules::RemapPermutation(Permutation);
}

// -------------------------------------------------------------------------------------------
// Point light shadows
// -------------------------------------------------------------------------------------------

class FPointLightPassKind : SHADER_PERMUTATION_ENUM("POINTLIGHT_PASS_KIND", ECubeMapRenderPassType);

struct FPointLightShadowRules
{
    using FPermutation = TShaderPermutation<FParallax, FClipping, FAlphaMask, FBindless, FPointLightPassKind, FRequiredAttributes>;

    static_assert(FPermutation::PermutationCount == 1024, "PointLightShadows permutation space grew unexpectedly");

    using FPassKind = FPointLightPassKind;

    NODISCARD static FPermutation Create(const FMaterialFeatures& Features, bool bBindless, FPassKind::Type PassKind)
    {
        return MakeShadowPermutation<FPointLightShadowRules, FPassKind>(Features, bBindless, PassKind);
    }

    NODISCARD static FPermutation RemapPermutation(FPermutation Permutation)
    {
        return RemapDepthOnlyAttributes(RemapMaterialPermutation(Permutation));
    }

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        if (Permutation.Get<FPointLightPassKind>() == ECubeMapRenderPassType::Unknown)
        {
            return false;
        }

        if (Permutation.Get<FBindless>() && !Desc.bSupportsBindless)
        {
            return false;
        }

        return RemapPermutation(Permutation) == Permutation;
    }
};

class FPointLightShadowVS : public FPointLightShadowRules
{
    DECLARE_SHADER_TYPE(FPointLightShadowVS, EShaderStage::Vertex);
};

class FPointLightShadowGS : public FPointLightShadowRules
{
    DECLARE_SHADER_TYPE(FPointLightShadowGS, EShaderStage::Geometry);

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        return Permutation.Get<FPointLightPassKind>() == ECubeMapRenderPassType::GeometryShaderSinglePass
            && FPointLightShadowRules::ShouldCompilePermutation(Desc);
    }
};

class FPointLightShadowPS : public FPointLightShadowRules
{
    DECLARE_SHADER_TYPE(FPointLightShadowPS, EShaderStage::Pixel);
};

// -------------------------------------------------------------------------------------------
// Cascaded shadows
// -------------------------------------------------------------------------------------------

class FCascadePassKind : SHADER_PERMUTATION_ENUM("CASCADE_PASS_KIND", ECascadeRenderPassType);

struct FCascadeShadowRules
{
    using FPermutation = TShaderPermutation<FParallax, FClipping, FAlphaMask, FBindless, FCascadePassKind, FRequiredAttributes>;

    static_assert(FPermutation::PermutationCount == 1280, "CascadedShadows permutation space grew unexpectedly");

    using FPassKind = FCascadePassKind;

    NODISCARD static FPermutation Create(const FMaterialFeatures& Features, bool bBindless, FPassKind::Type PassKind)
    {
        return MakeShadowPermutation<FCascadeShadowRules, FPassKind>(Features, bBindless, PassKind);
    }

    NODISCARD static FPermutation RemapPermutation(FPermutation Permutation)
    {
        return RemapDepthOnlyAttributes(RemapMaterialPermutation(Permutation));
    }

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        if (Permutation.Get<FCascadePassKind>() == ECascadeRenderPassType::Unknown)
        {
            return false;
        }

        if (Permutation.Get<FBindless>() && !Desc.bSupportsBindless)
        {
            return false;
        }

        return RemapPermutation(Permutation) == Permutation;
    }
};

class FCascadeShadowVS : public FCascadeShadowRules
{
    DECLARE_SHADER_TYPE(FCascadeShadowVS, EShaderStage::Vertex);
};

class FCascadeShadowGS : public FCascadeShadowRules
{
    DECLARE_SHADER_TYPE(FCascadeShadowGS, EShaderStage::Geometry);

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        return Permutation.Get<FCascadePassKind>() == ECascadeRenderPassType::GeometryShaderSinglePass
            && FCascadeShadowRules::ShouldCompilePermutation(Desc);
    }
};

class FCascadeShadowPS : public FCascadeShadowRules
{
    DECLARE_SHADER_TYPE(FCascadeShadowPS, EShaderStage::Pixel);
};

class FCascadeMatrixGenCS
{
    DECLARE_SHADER_TYPE(FCascadeMatrixGenCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

// -------------------------------------------------------------------------------------------
// Directional shadow mask
// -------------------------------------------------------------------------------------------

class FCSMFilterFunctionDim     : SHADER_PERMUTATION_ENUM("SHADOW_FILTER_FUNCTION", ECSMFilterFunction);
class FCSMFilterModeDim         : SHADER_PERMUTATION_ENUM("SHADOW_FILTER_MODE", ECSMFilterMode);
class FCSMDebug                 : SHADER_PERMUTATION_BOOL("ENABLE_DEBUG");
class FCSMRotateSamples         : SHADER_PERMUTATION_BOOL("ROTATE_SAMPLES");
class FCSMCascadeFromProjection : SHADER_PERMUTATION_BOOL("SELECT_CASCADE_FROM_PROJECTION");
class FCSMBlendCascades         : SHADER_PERMUTATION_BOOL("ENABLE_CASCADE_BLENDING");

class FCSMNumSamples : public TShaderPermutationInt<int32, 4>
{
    SHADER_PERMUTATION_DEFINE("NUM_SAMPLES");

public:

    /** @return The bucket whose table is large enough for the requested count. */
    NODISCARD static Type FromSampleCount(int32 NumSamples)
    {
        if (NumSamples <= 16)
        {
            return 0;
        }
        else if (NumSamples <= 32)
        {
            return 1;
        }
        else if (NumSamples <= 64)
        {
            return 2;
        }

        return 3;
    }

    NODISCARD static String ToDefineValue(Type Value)
    {
        return String::CreateFormatted("%d", 16 << Value);
    }
};

class FShadowMaskCS
{
    DECLARE_SHADER_TYPE(FShadowMaskCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<
        FCSMFilterFunctionDim,
        FCSMFilterModeDim,
        FCSMDebug,
        FCSMRotateSamples,
        FCSMCascadeFromProjection,
        FCSMBlendCascades,
        FCSMNumSamples>;

    static_assert(FPermutation::PermutationCount == 384, "ShadowMask permutation space grew unexpectedly");

    NODISCARD static FPermutation RemapPermutation(FPermutation Permutation)
    {
        if (Permutation.Get<FCSMFilterFunctionDim>() == ECSMFilterFunction::VogelDisk)
        {
            Permutation.Set<FCSMNumSamples>(FCSMNumSamples::FromSampleCount(32));
        }

        return Permutation;
    }

    NODISCARD static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FPermutation Permutation = FPermutation(Desc.PermutationID);
        return RemapPermutation(Permutation) == Permutation;
    }
};
