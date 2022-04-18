#pragma once
#include "RHIResources.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIShaderStage

enum class EShaderStage
{
    Vertex        = 1,
    Hull          = 2,
    Domain        = 3,
    Geometry      = 4,
    Mesh          = 5,
    Amplification = 6,
    Pixel         = 7,
    Compute       = 8,
    RayGen        = 9,
    RayAnyHit     = 10,
    RayClosestHit = 11,
    RayMiss       = 12,
};

inline const char* ToString(EShaderStage ShaderStage)
{
    switch(ShaderStage)
    {
    case EShaderStage::Vertex:        return "Vertex";
    case EShaderStage::Hull:          return "Hull";
    case EShaderStage::Domain:        return "Domain";
    case EShaderStage::Geometry:      return "Geometry";
    case EShaderStage::Mesh:          return "Mesh";
    case EShaderStage::Amplification: return "Amplification";
    case EShaderStage::Pixel:         return "Pixel";
    case EShaderStage::Compute:       return "Compute";
    case EShaderStage::RayGen:        return "RayGen";
    case EShaderStage::RayAnyHit:     return "RayAnyHit";
    case EShaderStage::RayClosestHit: return "RayClosestHit";
    case EShaderStage::RayMiss:       return "RayMiss";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIShaderParameterInfo

struct SRHIShaderParameterInfo
{
    uint32 NumConstantBuffers      = 0;
    uint32 NumShaderResourceViews  = 0;
    uint32 NumUnorderedAccessViews = 0;
    uint32 NumSamplerStates        = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShader

class CRHIShader : public CRHIResource
{
public:

    CRHIShader(EShaderStage InShaderStage)
        : CRHIResource()
        , ShaderStage(InShaderStage)
    { }

    virtual void GetShaderParameterInfo(SRHIShaderParameterInfo& OutShaderParameterInfo) const = 0;

    virtual bool GetConstantBufferIndexByName(const String& InName, uint32& OutIndex) const = 0;
    virtual bool GetUnorderedAccessViewIndexByName(const String& InName, uint32& OutIndex) const = 0;
    virtual bool GetShaderResourceViewIndexByName(const String& InName, uint32& OutIndex) const = 0;
    virtual bool GetSamplerIndexByName(const String& InName, uint32& OutIndex) const = 0;

    EShaderStage GetShaderStage() const { return ShaderStage; }

private:
    EShaderStage ShaderStage;
};

class CRHIComputeShader : public CRHIShader
{
public:
    CRHIComputeShader()
        : CRHIShader(EShaderStage::Compute)
    { }

    virtual CIntVector3 GetThreadGroupXYZ() const { return CIntVector3(1, 1, 1); };
};

class CRHIVertexShader : public CRHIShader
{
public:
    CRHIVertexShader()
        : CRHIShader(EShaderStage::Vertex)
    { }
};

class CRHIHullShader : public CRHIShader
{
public:
    CRHIHullShader()
        : CRHIShader(EShaderStage::Hull)
    { }
};

class CRHIDomainShader : public CRHIShader
{
public:
    CRHIDomainShader()
        : CRHIShader(EShaderStage::Domain)
    { }
};

class CRHIGeometryShader : public CRHIShader
{
public:
    CRHIGeometryShader()
        : CRHIShader(EShaderStage::Geometry)
    { }
};

class CRHIMeshShader : public CRHIShader
{
public:
    CRHIMeshShader()
        : CRHIShader(EShaderStage::Mesh)
    { }
};

class CRHIAmplificationShader : public CRHIShader
{
public:
    CRHIAmplificationShader()
        : CRHIShader(EShaderStage::Amplification)
    { }
};

class CRHIPixelShader : public CRHIShader
{
public:
    CRHIPixelShader()
        : CRHIShader(EShaderStage::Pixel)
    { }
};

class CRHIRayGenShader : public CRHIShader
{
public:
    CRHIRayGenShader()
        : CRHIShader(EShaderStage::RayGen)
    { }
};

class CRHIRayAnyHitShader : public CRHIShader
{
public:
    CRHIRayAnyHitShader()
        : CRHIShader(EShaderStage::RayAnyHit)
    { }
};

class CRHIRayClosestHitShader : public CRHIShader
{
public:
    CRHIRayClosestHitShader()
        : CRHIShader(EShaderStage::RayClosestHit)
    { }
};

class CRHIRayMissShader : public CRHIShader
{
public:
    CRHIRayMissShader()
        : CRHIShader(EShaderStage::RayMiss)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool ShaderStageIsGraphics(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case EShaderStage::Vertex:
    case EShaderStage::Hull:
    case EShaderStage::Domain:
    case EShaderStage::Geometry:
    case EShaderStage::Pixel:
    case EShaderStage::Mesh:
    case EShaderStage::Amplification:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}

inline bool ShaderStageIsCompute(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case EShaderStage::Compute:
    case EShaderStage::RayGen:
    case EShaderStage::RayClosestHit:
    case EShaderStage::RayAnyHit:
    case EShaderStage::RayMiss:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}

inline bool ShaderStageIsRayTracing(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case EShaderStage::RayGen:
    case EShaderStage::RayClosestHit:
    case EShaderStage::RayAnyHit:
    case EShaderStage::RayMiss:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}