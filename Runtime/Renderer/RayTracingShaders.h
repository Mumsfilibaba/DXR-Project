#pragma once
#include "RendererCore/Shaders/CommonShaderPermutations.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/Shaders/ShaderType.h"

// Mirrors MATERIAL_SRV_REGISTER_BASE in Assets/Shaders/ClosestHit.hlsl.
constexpr uint32 RAY_TRACING_MATERIAL_SLOT_REGISTER = 2;

class FRayTracingSER : SHADER_PERMUTATION_BOOL("RAY_TRACING_SHADER_EXECUTION_REORDERING");

using FRayTracingPermutation = TShaderPermutation<FBindless, FRayTracingSER>;

template<typename ShaderType>
struct TRayTracingShaderRules
{
    static bool ShouldCompilePermutation(const FShaderPermutationDesc& Desc)
    {
        const FRayTracingPermutation Permutation = FRayTracingPermutation(Desc.PermutationID);
        if (Permutation.Get<FRayTracingSER>())
        {
            return Permutation.Get<FBindless>() && Desc.bSupportsShaderExecutionReordering;
        }

        return !Permutation.Get<FBindless>() || Desc.bSupportsBindless;
    }

    static void ModifyCompilationEnvironment(const FShaderPermutationDesc& Desc, FShaderCompilationEnvironment& Environment)
    {
        if (FRayTracingPermutation(Desc.PermutationID).Get<FRayTracingSER>())
        {
            Environment.RequireShaderModel(EShaderModel::SM_6_9);
        }
    }
};

class FRayGenShader : public TRayTracingShaderRules<FRayGenShader>
{
    DECLARE_SHADER_TYPE(FRayGenShader, EShaderStage::RayGen);

    using FPermutation = FRayTracingPermutation;
};

class FRayMissShader : public TRayTracingShaderRules<FRayMissShader>
{
    DECLARE_SHADER_TYPE(FRayMissShader, EShaderStage::RayMiss);

    using FPermutation = FRayTracingPermutation;
};

class FRayClosestHitShader : public TRayTracingShaderRules<FRayClosestHitShader>
{
    DECLARE_SHADER_TYPE(FRayClosestHitShader, EShaderStage::RayClosestHit);

    using FPermutation = FRayTracingPermutation;
};

class FInlineReflectionsCS
{
    DECLARE_SHADER_TYPE(FInlineReflectionsCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

class FPrimaryRayDebugCS
{
    DECLARE_SHADER_TYPE(FPrimaryRayDebugCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

class FReflectionTemporalCS
{
    DECLARE_SHADER_TYPE(FReflectionTemporalCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

class FReflectionAtrousCS
{
    DECLARE_SHADER_TYPE(FReflectionAtrousCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

class FReflectionUpsampleCS
{
    DECLARE_SHADER_TYPE(FReflectionUpsampleCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};
