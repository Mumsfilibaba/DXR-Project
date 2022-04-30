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

class CNullRHIInputLayoutState : public CRHIInputLayoutState
{
public:
    CNullRHIInputLayoutState() = default;
    ~CNullRHIInputLayoutState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIDepthStencilState

class CNullRHIDepthStencilState : public CRHIDepthStencilState
{
public:
    CNullRHIDepthStencilState() = default;
    ~CNullRHIDepthStencilState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRasterizerState

class CNullRHIRasterizerState : public CRHIRasterizerState
{
public:
    CNullRHIRasterizerState() = default;
    ~CNullRHIRasterizerState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIBlendState

class CNullRHIBlendState : public CRHIBlendState
{
public:
    CNullRHIBlendState() = default;
    ~CNullRHIBlendState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGraphicsPipelineState

class CNullRHIGraphicsPipelineState : public CRHIGraphicsPipelineState
{
public:
    CNullRHIGraphicsPipelineState() = default;
    ~CNullRHIGraphicsPipelineState() = default;

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIComputePipelineState

class CNullRHIComputePipelineState : public CRHIComputePipelineState
{
public:
    CNullRHIComputePipelineState() = default;
    ~CNullRHIComputePipelineState() = default;

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingPipelineState

class CNullRHIRayTracingPipelineState : public CRHIRayTracingPipelineState
{
public:
    CNullRHIRayTracingPipelineState() = default;
    ~CNullRHIRayTracingPipelineState() = default;

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
    }

    virtual bool IsValid() const override final
    {
        return true;
    }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
