#pragma once
#include "RHI/RHIResources.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

struct FNullRHIBuffer
    : public FRHIBuffer
{
    explicit FNullRHIBuffer(const FRHIBufferDesc& InDesc)
        : FRHIBuffer(InDesc)
    { }

    virtual void* GetRHIBaseBuffer()         override final { return this; }
    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final{ return FRHIDescriptorHandle(); }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif