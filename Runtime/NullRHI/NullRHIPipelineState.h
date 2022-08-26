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
// CNullRHIInputLayoutState

class CNullRHIInputLayoutState : public FRHIVertexInputLayout
{
public:

    CNullRHIInputLayoutState()  = default;
    ~CNullRHIInputLayoutState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIDepthStencilState

class CNullRHIDepthStencilState : public FRHIDepthStencilState
{
public:

    CNullRHIDepthStencilState()  = default;
    ~CNullRHIDepthStencilState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRasterizerState

class CNullRHIRasterizerState : public FRHIRasterizerState
{
public:

    CNullRHIRasterizerState()  = default;
    ~CNullRHIRasterizerState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIBlendState

class CNullRHIBlendState : public FRHIBlendState
{
public:

    CNullRHIBlendState()  = default;
    ~CNullRHIBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGraphicsPipelineState

class CNullRHIGraphicsPipelineState : public FRHIGraphicsPipelineState
{
public:

    CNullRHIGraphicsPipelineState()  = default;
    ~CNullRHIGraphicsPipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIComputePipelineState

class CNullRHIComputePipelineState : public FRHIComputePipelineState
{
public:

    CNullRHIComputePipelineState()  = default;
    ~CNullRHIComputePipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingPipelineState

class CNullRHIRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:

    CNullRHIRayTracingPipelineState()  = default;
    ~CNullRHIRayTracingPipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
