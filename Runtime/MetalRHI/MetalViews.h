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
// CMetalShaderResourceView

class CMetalShaderResourceView : public CRHIShaderResourceView
{
public:

    explicit CMetalShaderResourceView(CRHIResource* InResource)
        : CRHIShaderResourceView(InResource)
    { }

    ~CMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalUnorderedAccessView

class CMetalUnorderedAccessView : public CRHIUnorderedAccessView
{
public:

    explicit CMetalUnorderedAccessView(CRHIResource* InResource)
        : CRHIUnorderedAccessView(InResource)
    { }

    ~CMetalUnorderedAccessView() = default;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
