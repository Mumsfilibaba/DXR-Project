#pragma once
#include "CoreRHI/RHIResources.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CNullRHIVertexShader : public CRHIVertexShader
{
public:
    CNullRHIVertexShader()
        : CRHIVertexShader()
    {
    }
};

class CNullRHIPixelShader : public CRHIPixelShader
{
public:
    CNullRHIPixelShader()
        : CRHIPixelShader()
    {
    }
};

class CNullRHIRayGenShader : public CRHIRayGenShader
{
public:
    CNullRHIRayGenShader()
        : CRHIRayGenShader()
    {
    }
};

class CNullRHIRayAnyhitShader : public CRHIRayAnyHitShader
{
public:
    CNullRHIRayAnyhitShader()
        : CRHIRayAnyHitShader()
    {
    }
};

class CNullRHIRayClosestHitShader : public CRHIRayClosestHitShader
{
public:
    CNullRHIRayClosestHitShader()
        : CRHIRayClosestHitShader()
    {
    }
};

class CNullRHIRayMissShader : public CRHIRayMissShader
{
public:
    CNullRHIRayMissShader()
        : CRHIRayMissShader()
    {
    }
};

class CNullRHIComputeShader : public CRHIComputeShader
{
public:
    CNullRHIComputeShader()
        : CRHIComputeShader()
    {
    }

    virtual CIntVector3 GetThreadGroupXYZ() const override
    {
        return CIntVector3();
    }
};

template<typename TBaseShader>
class TNullRHIShader : public TBaseShader
{
public:
    TNullRHIShader()
        : TBaseShader()
    {
    }

    virtual void GetShaderParameterInfo( SShaderParameterInfo& OutShaderParameterInfo ) const override
    {
        OutShaderParameterInfo = SShaderParameterInfo();
    }

    virtual bool GetShaderResourceViewIndexByName( const CString& InName, uint32& OutIndex ) const override
    {
        return true;
    }

    virtual bool GetSamplerIndexByName( const CString& InName, uint32& OutIndex ) const override
    {
        return true;
    }

    virtual bool GetUnorderedAccessViewIndexByName( const CString& InName, uint32& OutIndex ) const override
    {
        return true;
    }

    virtual bool GetConstantBufferIndexByName( const CString& InName, uint32& OutIndex ) const override
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
