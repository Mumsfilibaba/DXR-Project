#pragma once
#include "VulkanDeviceObject.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanComputeShader

class CVulkanComputeShader : public CRHIComputeShader
{
public:
    CVulkanComputeShader()
        : CRHIComputeShader()
    {
    }

    virtual CIntVector3 GetThreadGroupXYZ() const override
    {
        return CIntVector3(1, 1, 1);
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TVulkanShader

template<typename BaseShaderType>
class TVulkanShader : public BaseShaderType
{
public:
    TVulkanShader()
        : BaseShaderType()
    {
    }

    virtual void GetShaderParameterInfo(SRHIShaderParameterInfo& OutShaderParameterInfo) const override
    {
        OutShaderParameterInfo = SRHIShaderParameterInfo();
    }

    virtual bool GetShaderResourceViewIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return true;
    }

    virtual bool GetSamplerIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return true;
    }

    virtual bool GetUnorderedAccessViewIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return true;
    }

    virtual bool GetConstantBufferIndexByName(const String& InName, uint32& OutIndex) const override
    {
        return true;
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

