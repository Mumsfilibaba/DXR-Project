#pragma once
#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIBuffer

class CNullRHIBuffer : public CRHIBuffer
{
public:
    CNullRHIBuffer(const CRHIBufferDesc& InBufferDesc)
        : CRHIBuffer(InBufferDesc)
    { }

    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);
    }

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