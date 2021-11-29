#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"

enum class EShaderStage
{
    Vertex = 1,
    Hull = 2,
    Domain = 3,
    Geometry = 4,
    Mesh = 5,
    Amplification = 6,
    Pixel = 7,
    Compute = 8,
    RayGen = 9,
    RayAnyHit = 10,
    RayClosestHit = 11,
    RayMiss = 12,
};

struct SShaderParameterInfo
{
    uint32 NumConstantBuffers = 0;
    uint32 NumShaderResourceViews = 0;
    uint32 NumUnorderedAccessViews = 0;
    uint32 NumSamplerStates = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHIShader : public CRHIResource
{
public:

    virtual class CRHIVertexShader* AsVertexShader() { return nullptr; }
    virtual class CRHIHullShader* AsHullShader() { return nullptr; }
    virtual class CRHIDomainShader* AsDomainShader() { return nullptr; }
    virtual class CRHIGeometryShader* AsGeometryShader() { return nullptr; }
    virtual class CRHIPixelShader* AsPixelShader() { return nullptr; }

    virtual class CRHIComputeShader* AsComputeShader() { return nullptr; }

    virtual class CRHIRayGenShader* AsRayGenShader() { return nullptr; }
    virtual class CRHIRayAnyHitShader* AsRayAnyHitShader() { return nullptr; }
    virtual class CRHIRayClosestHitShader* AsRayClosestHitShader() { return nullptr; }
    virtual class CRHIRayMissShader* AsRayMissShader() { return nullptr; }

    virtual void GetShaderParameterInfo( SShaderParameterInfo& OutShaderParameterInfo ) const = 0;

    // Returns false if no parameter with the specified name exists
    virtual bool GetConstantBufferIndexByName( const CString& InName, uint32& OutIndex ) const = 0;
    virtual bool GetUnorderedAccessViewIndexByName( const CString& InName, uint32& OutIndex ) const = 0;
    virtual bool GetShaderResourceViewIndexByName( const CString& InName, uint32& OutIndex ) const = 0;
    virtual bool GetSamplerIndexByName( const CString& InName, uint32& OutIndex ) const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHIComputeShader : public CRHIShader
{
public:

    virtual CRHIComputeShader* AsComputeShader() { return this; }

    virtual CIntVector3 GetThreadGroupXYZ() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHIVertexShader : public CRHIShader
{
public:
    virtual CRHIVertexShader* AsVertexShader() { return this; }
};

class CRHIHullShader : public CRHIShader
{
public:
    virtual CRHIHullShader* AsHullShader() { return this; }
};

class CRHIDomainShader : public CRHIShader
{
public:
    virtual CRHIDomainShader* AsDomainShader() { return this; }
};

class CRHIGeometryShader : public CRHIShader
{
public:
    virtual CRHIGeometryShader* AsGeometryShader() { return this; }
};

class CRHIMeshShader : public CRHIShader
{
    // TODO
};

class CRHIAmplificationShader : public CRHIShader
{
    // TODO
};

class CRHIPixelShader : public CRHIShader
{
public:
    virtual CRHIPixelShader* AsPixelShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHIRayGenShader : public CRHIShader
{
public:
    virtual CRHIRayGenShader* AsRayGenShader() { return this; }
};

class CRHIRayAnyHitShader : public CRHIShader
{
public:
    virtual CRHIRayAnyHitShader* AsRayAnyHitShader() { return this; }
};

class CRHIRayClosestHitShader : public CRHIShader
{
public:
    virtual CRHIRayClosestHitShader* AsRayClosestHitShader() { return this; }
};

class CRHIRayMissShader : public CRHIShader
{
public:
    virtual CRHIRayMissShader* AsRayMissShader() { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

// Helpers
inline bool ShaderStageIsGraphics( EShaderStage ShaderStage )
{
    switch ( ShaderStage )
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

inline bool ShaderStageIsCompute( EShaderStage ShaderStage )
{
    switch ( ShaderStage )
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

inline bool ShaderStageIsRayTracing( EShaderStage ShaderStage )
{
    switch ( ShaderStage )
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