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

class CNullRHIComputeShader : public CRHIComputeShader
{
public:
    CNullRHIComputeShader()
        : CRHIComputeShader()
    {
    }

    virtual CIntVector3 GetThreadGroupXYZ() const override
    {
        return CIntVector3(1, 1, 1);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHIShader

template<typename BaseShaderType>
class TNullRHIShader : public BaseShaderType
{
public:
    TNullRHIShader()
        : BaseShaderType()
    {
    }

    virtual void GetShaderParameterInfo(SRHIShaderParameterInfo& OutShaderParameterInfo) const override
    {
        OutShaderParameterInfo = SRHIShaderParameterInfo();
    }

    virtual bool GetShaderResourceViewIndexByName(const CString& InName, uint32& OutIndex) const override
    {
        return true;
    }

    virtual bool GetSamplerIndexByName(const CString& InName, uint32& OutIndex) const override
    {
        return true;
    }

    virtual bool GetUnorderedAccessViewIndexByName(const CString& InName, uint32& OutIndex) const override
    {
        return true;
    }

    virtual bool GetConstantBufferIndexByName(const CString& InName, uint32& OutIndex) const override
    {
        return true;
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
