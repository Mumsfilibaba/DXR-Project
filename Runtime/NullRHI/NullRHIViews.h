#pragma once
#include "RHI/RHIResourceViews.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIShaderResourceView

class CNullRHIShaderResourceView : public CRHIShaderResourceView
{
public:
    CNullRHIShaderResourceView() = default;
    ~CNullRHIShaderResourceView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIUnorderedAccessView

class CNullRHIUnorderedAccessView : public CRHIUnorderedAccessView
{
public:
    CNullRHIUnorderedAccessView() = default;
    ~CNullRHIUnorderedAccessView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRenderTargetView

class CNullRHIRenderTargetView : public CRHIRenderTargetView
{
public:
    CNullRHIRenderTargetView() = default;
    ~CNullRHIRenderTargetView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIDepthStencilView

class CNullRHIDepthStencilView : public CRHIDepthStencilView
{
public:
    CNullRHIDepthStencilView() = default;
    ~CNullRHIDepthStencilView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};


#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
