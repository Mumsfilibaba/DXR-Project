#pragma once
#include "MetalObject.h"

#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalInputLayoutState

class CMetalInputLayoutState : public FRHIVertexInputLayout, public CMetalObject
{
public:

    CMetalInputLayoutState()  = default;
    ~CMetalInputLayoutState() = default;
    
private:

};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalDepthStencilState

class CMetalDepthStencilState : public FRHIDepthStencilState
{
public:

    CMetalDepthStencilState()  = default;
    ~CMetalDepthStencilState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRasterizerState

class CMetalRasterizerState : public FRHIRasterizerState
{
public:

    CMetalRasterizerState()  = default;
    ~CMetalRasterizerState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalBlendState

class CMetalBlendState : public FRHIBlendState
{
public:

    CMetalBlendState()  = default;
    ~CMetalBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalGraphicsPipelineState

class CMetalGraphicsPipelineState : public FRHIGraphicsPipelineState
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

class CMetalComputePipelineState : public FRHIComputePipelineState
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

class CMetalRayTracingPipelineState : public FRHIRayTracingPipelineState
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
