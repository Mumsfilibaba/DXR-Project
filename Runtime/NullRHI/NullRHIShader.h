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
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIComputeShader Interface

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
    // CRHIShader Interface

    virtual void* GetRHIBaseShader() override final { return this; }

    virtual void GetShaderParameterInfo(SShaderParameterInfo& OutShaderParameterInfo) const override final { OutShaderParameterInfo = SShaderParameterInfo(); }

    virtual bool GetShaderResourceViewIndexByName(const String& InName, uint32& OutIndex) const override final { return true; }

    virtual bool GetSamplerIndexByName(const String& InName, uint32& OutIndex) const override final { return true; }

    virtual bool GetUnorderedAccessViewIndexByName(const String& InName, uint32& OutIndex) const override final { return true; }

    virtual bool GetConstantBufferIndexByName(const String& InName, uint32& OutIndex) const override final { return true; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Deprecated

    virtual bool IsValid() const override final { return true; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
