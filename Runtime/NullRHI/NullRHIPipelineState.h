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
    FNullRHIInputLayoutState() = default;
};

struct FNullRHIDepthStencilState 
    : public FRHIDepthStencilState
{
    FNullRHIDepthStencilState() = default;
};

struct FNullRHIRasterizerState 
    : public FRHIRasterizerState
{
    FNullRHIRasterizerState() = default;
};

struct FNullRHIBlendState 
    : public FRHIBlendState
{
    FNullRHIBlendState() = default;
};

struct FNullRHIGraphicsPipelineState 
    : public FRHIGraphicsPipelineState
{
    FNullRHIGraphicsPipelineState() = default;

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

struct FNullRHIComputePipelineState 
    : public FRHIComputePipelineState
{
    FNullRHIComputePipelineState()  = default;
    ~FNullRHIComputePipelineState() = default;

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

struct FNullRHIRayTracingPipelineState 
    : public FRHIRayTracingPipelineState
{
    FNullRHIRayTracingPipelineState()  = default;
    ~FNullRHIRayTracingPipelineState() = default;

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
