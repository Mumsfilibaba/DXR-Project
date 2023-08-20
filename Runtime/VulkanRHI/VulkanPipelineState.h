#pragma once
#include "RHI/RHIResources.h"
#include "Core/Utilities/StringUtilities.h"

class FVulkanInputLayoutState : public FRHIVertexInputLayout
{
public:
    FVulkanInputLayoutState()  = default;
    ~FVulkanInputLayoutState() = default;
};

class FVulkanDepthStencilState : public FRHIDepthStencilState
{
public:
    FVulkanDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
        : FRHIDepthStencilState()
        , Desc(InDesc)
    {
    }

    virtual ~FVulkanDepthStencilState() = default;

    virtual FRHIDepthStencilStateDesc GetDesc() const override final { return Desc; }

private:
    FRHIDepthStencilStateDesc Desc;
};

class FVulkanRasterizerState : public FRHIRasterizerState
{
public:
    FVulkanRasterizerState(const FRHIRasterizerStateDesc& InDesc)
        : FRHIRasterizerState()
        , Desc(InDesc)
    {
    }

    virtual ~FVulkanRasterizerState() = default;

    virtual FRHIRasterizerStateDesc GetDesc() const override final { return Desc; }

private:
    FRHIRasterizerStateDesc Desc;
};

class FVulkanBlendState : public FRHIBlendState
{
public:
    FVulkanBlendState(const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState()
        , Desc(InDesc)
    {
    }

    virtual ~FVulkanBlendState() = default;

    virtual FRHIBlendStateDesc GetDesc() const override final { return Desc; }

private:
    FRHIBlendStateDesc Desc;
};

class FVulkanGraphicsPipelineState : public FRHIGraphicsPipelineState
{
public:
    FVulkanGraphicsPipelineState()  = default;
    ~FVulkanGraphicsPipelineState() = default;
};

class FVulkanComputePipelineState : public FRHIComputePipelineState
{
public:
    FVulkanComputePipelineState()  = default;
    ~FVulkanComputePipelineState() = default;
};

class FVulkanRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:
    FVulkanRayTracingPipelineState()  = default;
    ~FVulkanRayTracingPipelineState() = default;
};
