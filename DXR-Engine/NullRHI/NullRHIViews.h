#pragma once
#include "CoreRHI/RHIResourceViews.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

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
