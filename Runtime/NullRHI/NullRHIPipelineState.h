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

class CNullRHIInputLayoutState : public CRHIVertexInputLayout
{
public:

    CNullRHIInputLayoutState()  = default;
    ~CNullRHIInputLayoutState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIDepthStencilState

class CNullRHIDepthStencilState : public CRHIDepthStencilState
{
public:

    CNullRHIDepthStencilState()  = default;
    ~CNullRHIDepthStencilState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRasterizerState

class CNullRHIRasterizerState : public CRHIRasterizerState
{
public:

    CNullRHIRasterizerState()  = default;
    ~CNullRHIRasterizerState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIBlendState

class CNullRHIBlendState : public CRHIBlendState
{
public:

    CNullRHIBlendState()  = default;
    ~CNullRHIBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGraphicsPipelineState

class CNullRHIGraphicsPipelineState : public CRHIGraphicsPipelineState
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

class CNullRHIComputePipelineState : public CRHIComputePipelineState
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

class CNullRHIRayTracingPipelineState : public CRHIRayTracingPipelineState
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
