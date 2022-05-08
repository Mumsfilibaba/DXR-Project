#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalInputLayoutState

class CMetalInputLayoutState : public CRHIVertexInputLayout
{
public:

    CMetalInputLayoutState()  = default;
    ~CMetalInputLayoutState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalDepthStencilState

class CMetalDepthStencilState : public CRHIDepthStencilState
{
public:

    CMetalDepthStencilState()  = default;
    ~CMetalDepthStencilState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRasterizerState

class CMetalRasterizerState : public CRHIRasterizerState
{
public:

    CMetalRasterizerState()  = default;
    ~CMetalRasterizerState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalBlendState

class CMetalBlendState : public CRHIBlendState
{
public:

    CMetalBlendState()  = default;
    ~CMetalBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalGraphicsPipelineState

class CMetalGraphicsPipelineState : public CRHIGraphicsPipelineState
{
public:

    CMetalGraphicsPipelineState()  = default;
    ~CMetalGraphicsPipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalComputePipelineState

class CMetalComputePipelineState : public CRHIComputePipelineState
{
public:

    CMetalComputePipelineState()  = default;
    ~CMetalComputePipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayTracingPipelineState

class CMetalRayTracingPipelineState : public CRHIRayTracingPipelineState
{
public:

    CMetalRayTracingPipelineState()  = default;
    ~CMetalRayTracingPipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

#pragma clang diagnostic pop
