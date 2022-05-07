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

    explicit CNullRHIShaderResourceView(CRHIResource* InResource)
        : CRHIShaderResourceView(InResource)
    { }

    ~CNullRHIShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIUnorderedAccessView

class CNullRHIUnorderedAccessView : public CRHIUnorderedAccessView
{
public:

    explicit CNullRHIUnorderedAccessView(CRHIResource* InResource)
        : CRHIUnorderedAccessView(InResource)
    { }

    ~CNullRHIUnorderedAccessView() = default;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
