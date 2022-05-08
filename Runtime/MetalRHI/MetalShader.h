#pragma once
#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

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

#pragma clang diagnostic pop
