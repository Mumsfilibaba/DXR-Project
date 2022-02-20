#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanInputLayoutState

class CVulkanInputLayoutState : public CRHIInputLayoutState
{
public:
    CVulkanInputLayoutState() = default;
    ~CVulkanInputLayoutState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDepthStencilState

class CVulkanDepthStencilState : public CRHIDepthStencilState
{
public:
    CVulkanDepthStencilState() = default;
    ~CVulkanDepthStencilState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanRasterizerState

class CVulkanRasterizerState : public CRHIRasterizerState
{
public:
    CVulkanRasterizerState() = default;
    ~CVulkanRasterizerState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanBlendState

class CVulkanBlendState : public CRHIBlendState
{
public:
    CVulkanBlendState() = default;
    ~CVulkanBlendState() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanGraphicsPipelineState

class CVulkanGraphicsPipelineState : public CRHIGraphicsPipelineState
{
public:
    CVulkanGraphicsPipelineState() = default;
    ~CVulkanGraphicsPipelineState() = default;

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
// CVulkanComputePipelineState

class CVulkanComputePipelineState : public CRHIComputePipelineState
{
public:
    CVulkanComputePipelineState() = default;
    ~CVulkanComputePipelineState() = default;

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
// CVulkanRayTracingPipelineState

class CVulkanRayTracingPipelineState : public CRHIRayTracingPipelineState
{
public:
    CVulkanRayTracingPipelineState() = default;
    ~CVulkanRayTracingPipelineState() = default;

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
    }

    virtual bool IsValid() const override final
    {
        return true;
    }
};
