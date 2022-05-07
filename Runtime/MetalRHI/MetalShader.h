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
// CMetalComputeShader

class CMetalComputeShader : public CRHIComputeShader
{
public:

    CMetalComputeShader()
        : CRHIComputeShader()
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIComputeShader Interface

    virtual CIntVector3 GetThreadGroupXYZ() const override final { return CIntVector3(1, 1, 1); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMetalShader

template<typename BaseShaderType>
class TMetalShader : public BaseShaderType
{
public:

    TMetalShader()
        : BaseShaderType()
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseShader() override final { return this; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
