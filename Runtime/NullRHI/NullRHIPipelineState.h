#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

struct FNullRHIInputLayoutState
    : public FRHIVertexInputLayout
{
};

struct FNullRHIDepthStencilState
    : public FRHIDepthStencilState
{
};

struct FNullRHIRasterizerState
    : public FRHIRasterizerState
{
};

struct FNullRHIBlendState
    : public FRHIBlendState
{
};

struct FNullRHIGraphicsPipelineState
    : public FRHIGraphicsPipelineState
{
};

struct FNullRHIComputePipelineState
    : public FRHIComputePipelineState
{
};

struct FNullRHIRayTracingPipelineState
    : public FRHIRayTracingPipelineState
{
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
