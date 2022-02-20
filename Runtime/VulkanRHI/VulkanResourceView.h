#pragma once
#include "RHI/RHIResourceViews.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanShaderResourceView

class CVulkanShaderResourceView : public CRHIShaderResourceView
{
public:
    CVulkanShaderResourceView() = default;
    ~CVulkanShaderResourceView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanUnorderedAccessView

class CVulkanUnorderedAccessView : public CRHIUnorderedAccessView
{
public:
    CVulkanUnorderedAccessView() = default;
    ~CVulkanUnorderedAccessView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanRenderTargetView

class CVulkanRenderTargetView : public CRHIRenderTargetView
{
public:
    CVulkanRenderTargetView() = default;
    ~CVulkanRenderTargetView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDepthStencilView

class CVulkanDepthStencilView : public CRHIDepthStencilView
{
public:
    CVulkanDepthStencilView() = default;
    ~CVulkanDepthStencilView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};
