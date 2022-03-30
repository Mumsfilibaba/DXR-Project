#pragma once
#include "RHIResourceBase.h"

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
        : CRHIResource(ERHIResourceType::Shader)
        , ShaderStage(InShaderStage)
    { }

    virtual class CRHIVertexShader*   AsVertexShader()   { return nullptr; }
    virtual class CRHIHullShader*     AsHullShader()     { return nullptr; }
    virtual class CRHIDomainShader*   AsDomainShader()   { return nullptr; }
    virtual class CRHIGeometryShader* AsGeometryShader() { return nullptr; }
    virtual class CRHIPixelShader*    AsPixelShader()    { return nullptr; }

    virtual class CRHIComputeShader* AsComputeShader() { return nullptr; }

    virtual class CRHIRayGenShader*        AsRayGenShader()        { return nullptr; }
    virtual class CRHIRayAnyHitShader*     AsRayAnyHitShader()     { return nullptr; }
    virtual class CRHIRayClosestHitShader* AsRayClosestHitShader() { return nullptr; }
    virtual class CRHIRayMissShader*       AsRayMissShader()       { return nullptr; }

    virtual void GetShaderParameterInfo(SRHIShaderParameterInfo& OutShaderParameterInfo) const = 0;

    virtual bool GetConstantBufferIndexByName(const String& InName, uint32& OutIndex) const = 0;
    virtual bool GetUnorderedAccessViewIndexByName(const String& InName, uint32& OutIndex) const = 0;
    virtual bool GetShaderResourceViewIndexByName(const String& InName, uint32& OutIndex) const = 0;
    virtual bool GetSamplerIndexByName(const String& InName, uint32& OutIndex) const = 0;

    EShaderStage GetShaderStage() const { return ShaderStage; }

private:
    EShaderStage ShaderStage;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIComputeShader

class CRHIComputeShader : public CRHIShader
{
public:
    CRHIComputeShader()
        : CRHIShader(EShaderStage::Compute)
    { }

    virtual CRHIComputeShader* AsComputeShader() { return this; }

    virtual CIntVector3 GetThreadGroupXYZ() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexShader

class CRHIVertexShader : public CRHIShader
{
public:
    CRHIVertexShader()
        : CRHIShader(EShaderStage::Vertex)
    { }

    virtual CRHIVertexShader* AsVertexShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIHullShader

class CRHIHullShader : public CRHIShader
{
public:
    CRHIHullShader()
        : CRHIShader(EShaderStage::Hull)
    { }

    virtual CRHIHullShader* AsHullShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDomainShader

class CRHIDomainShader : public CRHIShader
{
public:
    CRHIDomainShader()
        : CRHIShader(EShaderStage::Domain)
    { }

    virtual CRHIDomainShader* AsDomainShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGeometryShader

class CRHIGeometryShader : public CRHIShader
{
public:
    CRHIGeometryShader()
        : CRHIShader(EShaderStage::Geometry)
    { }

    virtual CRHIGeometryShader* AsGeometryShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIMeshShader

class CRHIMeshShader : public CRHIShader
{
public:
    CRHIMeshShader()
        : CRHIShader(EShaderStage::Mesh)
    { }

    // TODO
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIAmplificationShader

class CRHIAmplificationShader : public CRHIShader
{
public:
    CRHIAmplificationShader()
        : CRHIShader(EShaderStage::Amplification)
    { }

    // TODO
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIPixelShader

class CRHIPixelShader : public CRHIShader
{
public:
    CRHIPixelShader()
        : CRHIShader(EShaderStage::Pixel)
    { }

    virtual CRHIPixelShader* AsPixelShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayGenShader

class CRHIRayGenShader : public CRHIShader
{
public:
    CRHIRayGenShader()
        : CRHIShader(EShaderStage::RayGen)
    { }

    virtual CRHIRayGenShader* AsRayGenShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayAnyHitShader

class CRHIRayAnyHitShader : public CRHIShader
{
public:
    CRHIRayAnyHitShader()
        : CRHIShader(EShaderStage::RayAnyHit)
    { }

    virtual CRHIRayAnyHitShader* AsRayAnyHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayClosestHitShader

class CRHIRayClosestHitShader : public CRHIShader
{
public:
    CRHIRayClosestHitShader()
        : CRHIShader(EShaderStage::RayClosestHit)
    { }

    virtual CRHIRayClosestHitShader* AsRayClosestHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayMissShader

class CRHIRayMissShader : public CRHIShader
{
public:
    CRHIRayMissShader()
        : CRHIShader(EShaderStage::RayMiss)
    { }

    virtual CRHIRayMissShader* AsRayMissShader() { return this; }
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