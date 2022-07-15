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
// FNullRHIShaderResourceView

struct FNullRHIShaderResourceView : public FRHIShaderResourceView
{ 
    explicit FNullRHIShaderResourceView(FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
    { }

    ~FNullRHIShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIUnorderedAccessView

struct FNullRHIUnorderedAccessView : public FRHIUnorderedAccessView
{
    explicit FNullRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
    { }

    ~FNullRHIUnorderedAccessView() = default;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
