#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIShader>                RHIShaderRef;

typedef TSharedRef<class CRHIVertexShader>          RHIVertexShaderRef;
typedef TSharedRef<class CRHIHullShader>            RHIHullShaderRef;
typedef TSharedRef<class CRHIDomainShader>          RHIDomainShaderRef;
typedef TSharedRef<class CRHIGeometryShader>        RHIGeometryShaderRef;
typedef TSharedRef<class CRHIMeshShader>            RHIMeshShaderRef;
typedef TSharedRef<class CRHIAmplificationShader>   RHIAmplificationShaderRef;
typedef TSharedRef<class CRHIPixelShader>           RHIPixelShaderRef;

typedef TSharedRef<class CRHIComputeShader>         RHIComputeShaderRef;

typedef TSharedRef<class CRHIRayTracingShader>      RHIRayTracingShaderRef;
typedef TSharedRef<class CRHIRayGenShader>          RHIRayGenShaderRef;
typedef TSharedRef<class CRHIRayMissShader>         RHIRayMissShaderRef;
typedef TSharedRef<class CRHIRayClosestHitShader>   RHIRayClosestHitShaderRef;
typedef TSharedRef<class CRHIRayAnyHitShader>       RHIRayAnyHitShaderRef;
typedef TSharedRef<class CRHIRayIntersectionShader> RHIRayIntersectionShaderRef;
typedef TSharedRef<class CRHIRayCallableShader>     RHIRayCallableShaderRef;

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
// CRHIShader

class CRHIShader : public CRHIResource
{
protected:

    explicit CRHIShader(EShaderStage InShaderStage)
        : ShaderStage(InShaderStage)
    { }

    ~CRHIShader() = default;

public:

    /** @return: Returns the native handle of the Shader */
    virtual void* GetRHIBaseResource() { return nullptr; }

    /** @return: Returns the RHI-backend Shader interface */
    virtual void* GetRHIBaseShader() { return nullptr; }

    /** @return: Returns ShaderStage that the shader can be bound to */
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

    ~CRHIComputeShader() = default;

public:

    /** @return: Returns a vector with the number of threads in each dimension */
    virtual CIntVector3 GetThreadGroupXYZ() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGraphicsShader

class CRHIGraphicsShader : public CRHIShader
{
protected:

    explicit CRHIGraphicsShader(EShaderStage InShaderStage)
        : CRHIShader(InShaderStage)
    { }

    ~CRHIGraphicsShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexShader

class CRHIVertexShader : public CRHIGraphicsShader
{
protected:

    CRHIVertexShader()
        : CRHIGraphicsShader(EShaderStage::Vertex)
    { }

    ~CRHIVertexShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIHullShader

class CRHIHullShader : public CRHIGraphicsShader
{
protected:

    CRHIHullShader()
        : CRHIGraphicsShader(EShaderStage::Hull)
    { }

    ~CRHIHullShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDomainShader

class CRHIDomainShader : public CRHIGraphicsShader
{
protected:

    CRHIDomainShader()
        : CRHIGraphicsShader(EShaderStage::Domain)
    { }

    ~CRHIDomainShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGeometryShader

class CRHIGeometryShader : public CRHIGraphicsShader
{
protected:

    CRHIGeometryShader()
        : CRHIGraphicsShader(EShaderStage::Geometry)
    { }

    ~CRHIGeometryShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIMeshShader

class CRHIMeshShader : public CRHIGraphicsShader
{
protected:

    CRHIMeshShader()
        : CRHIGraphicsShader(EShaderStage::Mesh)
    { }

    ~CRHIMeshShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIAmplificationShader

class CRHIAmplificationShader : public CRHIGraphicsShader
{
protected:

    CRHIAmplificationShader()
        : CRHIGraphicsShader(EShaderStage::Amplification)
    { }

    ~CRHIAmplificationShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIPixelShader

class CRHIPixelShader : public CRHIGraphicsShader
{
protected:

    CRHIPixelShader()
        : CRHIGraphicsShader(EShaderStage::Pixel)
    { }

    ~CRHIPixelShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingShader

class CRHIRayTracingShader : public CRHIShader
{
protected:

    explicit CRHIRayTracingShader(EShaderStage InShaderStage)
        : CRHIShader(InShaderStage)
    { }

    ~CRHIRayTracingShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayGenShader

class CRHIRayGenShader : public CRHIRayTracingShader
{
protected:

    CRHIRayGenShader()
        : CRHIRayTracingShader(EShaderStage::RayGen)
    { }

    ~CRHIRayGenShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayAnyHitShader

class CRHIRayAnyHitShader : public CRHIRayTracingShader
{
protected:

    CRHIRayAnyHitShader()
        : CRHIRayTracingShader(EShaderStage::RayAnyHit)
    { }

    ~CRHIRayAnyHitShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayClosestHitShader

class CRHIRayClosestHitShader : public CRHIRayTracingShader
{
protected:

    CRHIRayClosestHitShader()
        : CRHIRayTracingShader(EShaderStage::RayClosestHit)
    { }

    ~CRHIRayClosestHitShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayMissShader

class CRHIRayMissShader : public CRHIRayTracingShader
{
protected:

    CRHIRayMissShader()
        : CRHIRayTracingShader(EShaderStage::RayMiss)
    { }

    ~CRHIRayMissShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayIntersectionShader

class CRHIRayIntersectionShader : public CRHIRayTracingShader
{
protected:

    CRHIRayIntersectionShader()
        : CRHIRayTracingShader(EShaderStage::RayIntersection)
    { }

    ~CRHIRayIntersectionShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayCallableShader

class CRHIRayCallableShader : public CRHIRayTracingShader
{
protected:

    CRHIRayCallableShader()
        : CRHIRayTracingShader(EShaderStage::RayCallable)
    { }

    ~CRHIRayCallableShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

inline bool ShaderStageIsGraphics(EShaderStage ShaderStage)
{
    return ((ShaderStage >= EShaderStage::Vertex) && (ShaderStage < EShaderStage::Compute)) ? true : false;
}

/** @brief: Determine if the Compute Pipeline is used (DXR uses the compute pipeline for RootSignatures) */
inline bool ShaderStageIsCompute(EShaderStage ShaderStage)
{
    return (ShaderStage >= EShaderStage::Compute) ? true : false;
}

inline bool ShaderStageIsRayTracing(EShaderStage ShaderStage)
{
    return (ShaderStage >= EShaderStage::RayGen) ? true : false;
}