#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIShaderStage

enum class ERHIShaderStage
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

class CRHIShader : public CRHIObject
{
public:

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
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIComputeShader

class CRHIComputeShader : public CRHIShader
{
public:
    virtual CRHIComputeShader* AsComputeShader() { return this; }

    virtual CIntVector3 GetThreadGroupXYZ() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexShader

class CRHIVertexShader : public CRHIShader
{
public:
    virtual CRHIVertexShader* AsVertexShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIHullShader

class CRHIHullShader : public CRHIShader
{
public:
    virtual CRHIHullShader* AsHullShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDomainShader

class CRHIDomainShader : public CRHIShader
{
public:
    virtual CRHIDomainShader* AsDomainShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGeometryShader

class CRHIGeometryShader : public CRHIShader
{
public:
    virtual CRHIGeometryShader* AsGeometryShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIMeshShader

class CRHIMeshShader : public CRHIShader
{
    // TODO
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIAmplificationShader

class CRHIAmplificationShader : public CRHIShader
{
    // TODO
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIPixelShader

class CRHIPixelShader : public CRHIShader
{
public:
    virtual CRHIPixelShader* AsPixelShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayGenShader

class CRHIRayGenShader : public CRHIShader
{
public:
    virtual CRHIRayGenShader* AsRayGenShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayAnyHitShader

class CRHIRayAnyHitShader : public CRHIShader
{
public:
    virtual CRHIRayAnyHitShader* AsRayAnyHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayClosestHitShader

class CRHIRayClosestHitShader : public CRHIShader
{
public:
    virtual CRHIRayClosestHitShader* AsRayClosestHitShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayMissShader

class CRHIRayMissShader : public CRHIShader
{
public:
    virtual CRHIRayMissShader* AsRayMissShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool ShaderStageIsGraphics(ERHIShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case ERHIShaderStage::Vertex:
    case ERHIShaderStage::Hull:
    case ERHIShaderStage::Domain:
    case ERHIShaderStage::Geometry:
    case ERHIShaderStage::Pixel:
    case ERHIShaderStage::Mesh:
    case ERHIShaderStage::Amplification:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}

inline bool ShaderStageIsCompute(ERHIShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case ERHIShaderStage::Compute:
    case ERHIShaderStage::RayGen:
    case ERHIShaderStage::RayClosestHit:
    case ERHIShaderStage::RayAnyHit:
    case ERHIShaderStage::RayMiss:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}

inline bool ShaderStageIsRayTracing(ERHIShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
    case ERHIShaderStage::RayGen:
    case ERHIShaderStage::RayClosestHit:
    case ERHIShaderStage::RayAnyHit:
    case ERHIShaderStage::RayMiss:
    {
        return true;
    }

    default:
    {
        return false;
    }
    }
}