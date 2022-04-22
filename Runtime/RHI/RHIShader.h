#pragma once
#include "RHIResources.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIShader>                CRHIShaderRef;

typedef TSharedRef<class CRHIVertexShader>          CRHIVertexShaderRef;
typedef TSharedRef<class CRHIHullShader>            CRHIHullShaderRef;
typedef TSharedRef<class CRHIDomainShader>          CRHIDomainShaderRef;
typedef TSharedRef<class CRHIGeometryShader>        CRHIGeometryShaderRef;
typedef TSharedRef<class CRHIMeshShader>            CRHIMeshShaderRef;
typedef TSharedRef<class CRHIAmplificationShader>   CRHIAmplificationShaderRef;
typedef TSharedRef<class CRHIPixelShader>           CRHIPixelShaderRef;

typedef TSharedRef<class CRHIComputeShader>         CRHIComputeShaderRef;

typedef TSharedRef<class CRHIRayTracingShader>      CRHIRayTracingShaderRef;
typedef TSharedRef<class CRHIRayGenShader>          CRHIRayGenShaderRef;
typedef TSharedRef<class CRHIRayMissShader>         CRHIRayMissShaderRef;
typedef TSharedRef<class CRHIRayClosestHitShader>   CRHIRayClosestHitShaderRef;
typedef TSharedRef<class CRHIRayAnyHitShader>       CRHIRayAnyHitShaderRef;
typedef TSharedRef<class CRHIRayIntersectionShader> CRHIRayIntersectionShaderRef;
typedef TSharedRef<class CRHIRayCallableShader>     CRHIRayCallableShaderRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderStage

enum class EShaderStage : uint8
{
    // Graphics
    Vertex          = 1,
    Hull            = 2,
    Domain          = 3,
    Geometry        = 4,
    Mesh            = 5,
    Amplification   = 6,
    Pixel           = 7,

    // Compute
    Compute         = 8,
    
    // RayTracing
    RayGen          = 9,
    RayAnyHit       = 10,
    RayClosestHit   = 11,
    RayMiss         = 12,
    RayIntersection = 13,
    RayCallable     = 14
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
// SShaderParameterInfo

struct SShaderParameterInfo
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
protected:

    CRHIShader(EShaderStage InShaderStage)
        : CRHIResource()
        , ShaderStage(InShaderStage)
    { }

public:

    virtual void GetShaderParameterInfo(SShaderParameterInfo& OutShaderParameterInfo) const = 0;

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
protected:

    CRHIComputeShader()
        : CRHIShader(EShaderStage::Compute)
    { }

public:

    /** @return: Returns a vector with the ThreadGroup count of the shader */
    virtual CIntVector3 GetThreadGroupXYZ() const { return CIntVector3(1, 1, 1); };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexShader

class CRHIVertexShader : public CRHIShader
{
protected:
    
    CRHIVertexShader()
        : CRHIShader(EShaderStage::Vertex)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIHullShader

class CRHIHullShader : public CRHIShader
{
protected:
    
    CRHIHullShader()
        : CRHIShader(EShaderStage::Hull)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDomainShader

class CRHIDomainShader : public CRHIShader
{
protected:
    
    CRHIDomainShader()
        : CRHIShader(EShaderStage::Domain)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGeometryShader

class CRHIGeometryShader : public CRHIShader
{
protected:
    
    CRHIGeometryShader()
        : CRHIShader(EShaderStage::Geometry)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIMeshShader

class CRHIMeshShader : public CRHIShader
{
protected:
    
    CRHIMeshShader()
        : CRHIShader(EShaderStage::Mesh)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIAmplificationShader

class CRHIAmplificationShader : public CRHIShader
{
protected:
    
    CRHIAmplificationShader()
        : CRHIShader(EShaderStage::Amplification)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIPixelShader

class CRHIPixelShader : public CRHIShader
{
protected:

    CRHIPixelShader()
        : CRHIShader(EShaderStage::Pixel)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingShader

class CRHIRayTracingShader : public CRHIShader
{
protected:

    CRHIRayTracingShader(EShaderStage InShaderStage)
        : CRHIShader(InShaderStage)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayGenShader

class CRHIRayGenShader : public CRHIRayTracingShader
{
protected:
    
    CRHIRayGenShader()
        : CRHIRayTracingShader(EShaderStage::RayGen)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayAnyHitShader

class CRHIRayAnyHitShader : public CRHIRayTracingShader
{
protected:
    
    CRHIRayAnyHitShader()
        : CRHIRayTracingShader(EShaderStage::RayAnyHit)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayClosestHitShader

class CRHIRayClosestHitShader : public CRHIRayTracingShader
{
protected:

    CRHIRayClosestHitShader()
        : CRHIRayTracingShader(EShaderStage::RayClosestHit)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayMissShader

class CRHIRayMissShader : public CRHIRayTracingShader
{
protected:

    CRHIRayMissShader()
        : CRHIRayTracingShader(EShaderStage::RayMiss)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayIntersectionShader

class CRHIRayIntersectionShader : public CRHIRayTracingShader
{
protected:

    CRHIRayIntersectionShader()
        : CRHIRayTracingShader(EShaderStage::RayMiss)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayCallableShader

class CRHIRayCallableShader : public CRHIRayTracingShader
{
protected:

    CRHIRayCallableShader()
        : CRHIRayTracingShader(EShaderStage::RayMiss)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool ShaderStageIsGraphics(EShaderStage ShaderStage)
{
    return ((ShaderStage >= EShaderStage::Vertex) && (ShaderStage < EShaderStage::Compute)) ? true : false;
}

/**
 * @brief: Determine if the Compute Pipeline is used (DXR uses the compute pipeline for RootSignatures)
 */
inline bool ShaderStageIsCompute(EShaderStage ShaderStage)
{
    return (ShaderStage >= EShaderStage::Compute) ? true : false;
}

inline bool ShaderStageIsRayTracing(EShaderStage ShaderStage)
{
    return (ShaderStage >= EShaderStage::RayGen) ? true : false;
}