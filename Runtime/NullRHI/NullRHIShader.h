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
// CNullRHIComputeShader

class CNullRHIComputeShader : public FRHIComputeShader
{
public:

    CNullRHIComputeShader()
        : FRHIComputeShader()
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIComputeShader Interface

    virtual CIntVector3 GetThreadGroupXYZ() const override final { return CIntVector3(1, 1, 1); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHIShader

template<typename BaseShaderType>
class TNullRHIShader : public BaseShaderType
{
public:

    TNullRHIShader()
        : BaseShaderType()
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIShader Interface

    virtual void* GetRHIBaseShader() override final { return this; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
