#pragma once
#include "Core/Core.h"

enum class EShaderStage : uint8
{
    Unknown = 0,
    
    // Graphics
    Vertex        = 1,
    Hull          = 2,
    Domain        = 3,
    Geometry      = 4,
    Mesh          = 5,
    Amplification = 6,
    Pixel         = 7,

    // Compute
    Compute = 8,
    
    // RayTracing
    RayGen          = 9,
    RayAnyHit       = 10,
    RayClosestHit   = 11,
    RayMiss         = 12,
    RayIntersection = 13,
    RayCallable     = 14,
};

NODISCARD constexpr const CHAR_T* ToString(EShaderStage ShaderStage)
{
    switch(ShaderStage)
    {
        case EShaderStage::Vertex:          return TEXT("Vertex");
        case EShaderStage::Hull:            return TEXT("Hull");
        case EShaderStage::Domain:          return TEXT("Domain");
        case EShaderStage::Geometry:        return TEXT("Geometry");
        case EShaderStage::Mesh:            return TEXT("Mesh");
        case EShaderStage::Amplification:   return TEXT("Amplification");
        case EShaderStage::Pixel:           return TEXT("Pixel");
        case EShaderStage::Compute:         return TEXT("Compute");
        case EShaderStage::RayGen:          return TEXT("RayGen");
        case EShaderStage::RayAnyHit:       return TEXT("RayAnyHit");
        case EShaderStage::RayClosestHit:   return TEXT("RayClosestHit");
        case EShaderStage::RayMiss:         return TEXT("RayMiss");
        case EShaderStage::RayIntersection: return TEXT("RayIntersection");
        case EShaderStage::RayCallable:     return TEXT("RayCallable");
        default:                            return TEXT("Unknown");
    }
}

NODISCARD constexpr bool IsShaderStageGraphics(EShaderStage ShaderStage)
{
    return ShaderStage >= EShaderStage::Vertex && ShaderStage < EShaderStage::Compute ? true : false;
}

NODISCARD constexpr bool IsShaderStageCompute(EShaderStage ShaderStage)
{
    return ShaderStage >= EShaderStage::Compute ? true : false;
}

NODISCARD constexpr bool IsShaderStageRayTracing(EShaderStage ShaderStage)
{
    return ShaderStage >= EShaderStage::RayGen ? true : false;
}

enum class EShaderModel : uint8
{
    Unknown = 0,
    SM_5_0  = 1, // DXBC for D3D11RHI. SM 5.1 is D3D12-only and intentionally absent, 2 stays unused.
    SM_6_0  = 3,
    SM_6_1  = 4,
    SM_6_2  = 5,
    SM_6_3  = 6,
    SM_6_4  = 7,
    SM_6_5  = 8,
    SM_6_6  = 9,
    SM_6_7  = 10,
    SM_6_8  = 11,
    SM_6_9  = 12,
    SM_6_10 = 13,
};

NODISCARD constexpr const CHAR_T* ToString(EShaderModel ShaderModel)
{
    switch (ShaderModel)
    {
        case EShaderModel::SM_5_0:  return TEXT("SM_5_0");
        case EShaderModel::SM_6_0:  return TEXT("SM_6_0");
        case EShaderModel::SM_6_1:  return TEXT("SM_6_1");
        case EShaderModel::SM_6_2:  return TEXT("SM_6_2");
        case EShaderModel::SM_6_3:  return TEXT("SM_6_3");
        case EShaderModel::SM_6_4:  return TEXT("SM_6_4");
        case EShaderModel::SM_6_5:  return TEXT("SM_6_5");
        case EShaderModel::SM_6_6:  return TEXT("SM_6_6");
        case EShaderModel::SM_6_7:  return TEXT("SM_6_7");
        case EShaderModel::SM_6_8:  return TEXT("SM_6_8");
        case EShaderModel::SM_6_9:  return TEXT("SM_6_9");
        case EShaderModel::SM_6_10: return TEXT("SM_6_10");

        default: return TEXT("Unknown");
    }
}

enum class EShaderOutputLanguage : uint8
{
    Unknown = 0,

    /** DXIL for D3D12RHI and NullRHI */
    DXIL = 1,

    /** Metal Shading Language for MetalRHI */
    MSL = 2,

    /** SPIR-V for VulkanRHI */
    SPIRV = 3,

    /** Shader Model 5.0 DXBC for D3D11RHI */
    DXBC = 4,
};

NODISCARD constexpr const CHAR_T* ToString(EShaderOutputLanguage OutputLanguage)
{
    switch (OutputLanguage)
    {
        case EShaderOutputLanguage::DXIL:  return TEXT("DXIL");
        case EShaderOutputLanguage::MSL:   return TEXT("MSL");
        case EShaderOutputLanguage::SPIRV: return TEXT("SPIRV");
        case EShaderOutputLanguage::DXBC:  return TEXT("DXBC");
        default:                           return TEXT("Unknown");
    }
}

/** Replaces the RHI::IsRHISupportedByPlatform filter in FShaderCompiler::GetSupportedOutputLanguages */
NODISCARD constexpr bool IsShaderOutputLanguageSupportedByPlatform(EShaderOutputLanguage OutputLanguage)
{
    switch (OutputLanguage)
    {
        case EShaderOutputLanguage::DXIL:
        case EShaderOutputLanguage::DXBC:
        {
        #if PLATFORM_WINDOWS
            return true;
        #else
            return false;
        #endif
        }

        case EShaderOutputLanguage::MSL:
        {
        #if PLATFORM_MACOS
            return true;
        #else
            return false;
        #endif
        }

        case EShaderOutputLanguage::SPIRV:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}
