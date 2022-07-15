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

class FNullRHIInputLayoutState : public FRHIVertexInputLayout
{
public:

    FNullRHIInputLayoutState()  = default;
    ~FNullRHIInputLayoutState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIDepthStencilState

class FNullRHIDepthStencilState : public FRHIDepthStencilState
{
public:

    FNullRHIDepthStencilState()  = default;
    ~FNullRHIDepthStencilState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIRasterizerState

class FNullRHIRasterizerState : public FRHIRasterizerState
{
public:

    FNullRHIRasterizerState()  = default;
    ~FNullRHIRasterizerState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIBlendState

class FNullRHIBlendState : public FRHIBlendState
{
public:

    FNullRHIBlendState()  = default;
    ~FNullRHIBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGraphicsPipelineState

class CNullRHIGraphicsPipelineState : public FRHIGraphicsPipelineState
{
public:

    CNullRHIGraphicsPipelineState()  = default;
    ~CNullRHIGraphicsPipelineState() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIComputePipelineState

class FNullRHIComputePipelineState : public FRHIComputePipelineState
{
public:

    FNullRHIComputePipelineState()  = default;
    ~FNullRHIComputePipelineState() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void    SetName(const FString& InName) override final { }
    virtual FString GetName() const override final                { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIRayTracingPipelineState

class FNullRHIRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:

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
