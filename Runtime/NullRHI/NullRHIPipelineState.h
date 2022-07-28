#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIInputLayoutState

struct FNullRHIInputLayoutState 
    : public FRHIVertexInputLayout
{
    FNullRHIInputLayoutState()  = default;
    ~FNullRHIInputLayoutState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIDepthStencilState

struct FNullRHIDepthStencilState 
    : public FRHIDepthStencilState
{
    FNullRHIDepthStencilState()  = default;
    ~FNullRHIDepthStencilState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIRasterizerState

struct FNullRHIRasterizerState 
    : public FRHIRasterizerState
{
    FNullRHIRasterizerState()  = default;
    ~FNullRHIRasterizerState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIBlendState

struct FNullRHIBlendState 
    : public FRHIBlendState
{
    FNullRHIBlendState()  = default;
    ~FNullRHIBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIGraphicsPipelineState

struct FNullRHIGraphicsPipelineState 
    : public FRHIGraphicsPipelineState
{
    FNullRHIGraphicsPipelineState()  = default;
    ~FNullRHIGraphicsPipelineState() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIComputePipelineState

struct FNullRHIComputePipelineState 
    : public FRHIComputePipelineState
{
    FNullRHIComputePipelineState()  = default;
    ~FNullRHIComputePipelineState() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIRayTracingPipelineState

struct FNullRHIRayTracingPipelineState 
    : public FRHIRayTracingPipelineState
{
    FNullRHIRayTracingPipelineState()  = default;
    ~FNullRHIRayTracingPipelineState() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
