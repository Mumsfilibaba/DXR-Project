#pragma once
#include "CoreRHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

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

class CNullRHIGraphicsPipelineState : public CRHIGraphicsPipelineState
{
public:
    CNullRHIGraphicsPipelineState() = default;
    ~CNullRHIGraphicsPipelineState() = default;

    virtual void SetName( const CString& InName ) override final
    {
        CRHIResource::SetName( InName );
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

class CNullRHIComputePipelineState : public CRHIComputePipelineState
{
public:
    CNullRHIComputePipelineState() = default;
    ~CNullRHIComputePipelineState() = default;

    virtual void SetName( const CString& InName ) override final
    {
        CRHIResource::SetName( InName );
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

class CNullRHIRayTracingPipelineState : public CRHIRayTracingPipelineState
{
public:
    CNullRHIRayTracingPipelineState() = default;
    ~CNullRHIRayTracingPipelineState() = default;

    virtual void SetName( const CString& InName ) override
    {
        CRHIResource::SetName( InName );
    }

    virtual bool IsValid() const
    {
        return true;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
