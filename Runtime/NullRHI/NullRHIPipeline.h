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
// CNullRHIDepthStencilState

class CNullRHIDepthStencilState final : public CRHIDepthStencilState
{
public:

    CNullRHIDepthStencilState() = default;
    ~CNullRHIDepthStencilState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRasterizerState

class CNullRHIRasterizerState final : public CRHIRasterizerState
{
public:

    CNullRHIRasterizerState() = default;
    ~CNullRHIRasterizerState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIBlendState

class CNullRHIBlendState final : public CRHIBlendState
{
public:

    CNullRHIBlendState() = default;
    ~CNullRHIBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIVertexInputLayout

class CNullRHIVertexInputLayout final : public CRHIVertexInputLayout
{
public:

    CNullRHIVertexInputLayout() = default;
    ~CNullRHIVertexInputLayout() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGraphicsPipelineState

class CNullRHIGraphicsPipelineState final : public CRHIGraphicsPipelineState
{
public:

    CNullRHIGraphicsPipelineState() = default;
    ~CNullRHIGraphicsPipelineState() = default;

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIComputePipelineState

class CRHIComputePipelineState final : public CRHIPipelineState
{
public:

    CRHIComputePipelineState() = default;
    ~CRHIComputePipelineState() = default;

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingPipelineState

class CRHIRayTracingPipelineState final : public CRHIPipelineState
{
public:

    CRHIRayTracingPipelineState() = default;
    ~CRHIRayTracingPipelineState() = default;

    virtual void   SetName(const String& InName) override final { }
    virtual String GetName() const override final { return ""; }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
