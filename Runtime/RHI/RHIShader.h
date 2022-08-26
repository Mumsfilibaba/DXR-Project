#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class FRHIShader>                RHIShaderRef;

typedef TSharedRef<class FRHIVertexShader>          RHIVertexShaderRef;
typedef TSharedRef<class CRHIHullShader>            RHIHullShaderRef;
typedef TSharedRef<class CRHIDomainShader>          RHIDomainShaderRef;
typedef TSharedRef<class CRHIGeometryShader>        RHIGeometryShaderRef;
typedef TSharedRef<class CRHIMeshShader>            RHIMeshShaderRef;
typedef TSharedRef<class CRHIAmplificationShader>   RHIAmplificationShaderRef;
typedef TSharedRef<class FRHIPixelShader>           RHIPixelShaderRef;

typedef TSharedRef<class FRHIComputeShader>         RHIComputeShaderRef;

typedef TSharedRef<class FRHIRayTracingShader>      RHIRayTracingShaderRef;
typedef TSharedRef<class FRHIRayGenShader>          RHIRayGenShaderRef;
typedef TSharedRef<class FRHIRayMissShader>         RHIRayMissShaderRef;
typedef TSharedRef<class FRHIRayClosestHitShader>   RHIRayClosestHitShaderRef;
typedef TSharedRef<class FRHIRayAnyHitShader>       RHIRayAnyHitShaderRef;
typedef TSharedRef<class CRHIRayIntersectionShader> RHIRayIntersectionShaderRef;
typedef TSharedRef<class CRHIRayCallableShader>     RHIRayCallableShaderRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderStage

enum class EShaderStage : uint8
{
    Unknown         = 0,
    
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
    RayCallable     = 14,
};

inline const char* ToString(EShaderStage ShaderStage)
{
    switch(ShaderStage)
    {
        case EShaderStage::Vertex:          return "Vertex";
        case EShaderStage::Hull:            return "Hull";
        case EShaderStage::Domain:          return "Domain";
        case EShaderStage::Geometry:        return "Geometry";
        case EShaderStage::Mesh:            return "Mesh";
        case EShaderStage::Amplification:   return "Amplification";
        case EShaderStage::Pixel:           return "Pixel";
        case EShaderStage::Compute:         return "Compute";
        case EShaderStage::RayGen:          return "RayGen";
        case EShaderStage::RayAnyHit:       return "RayAnyHit";
        case EShaderStage::RayClosestHit:   return "RayClosestHit";
        case EShaderStage::RayMiss:         return "RayMiss";
		case EShaderStage::RayIntersection: return "RayIntersection";
		case EShaderStage::RayCallable:     return "RayCallable";
		default:                            return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIShader

class FRHIShader : public FRHIResource
{
protected:

    explicit FRHIShader(EShaderStage InShaderStage)
        : ShaderStage(InShaderStage)
    { }

    ~FRHIShader() = default;

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
// FRHIComputeShader

class FRHIComputeShader : public FRHIShader
{
protected:

    FRHIComputeShader()
        : FRHIShader(EShaderStage::Compute)
    { }

    ~FRHIComputeShader() = default;

public:

    /** @return: Returns a vector with the number of threads in each dimension */
    virtual FIntVector3 GetThreadGroupXYZ() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGraphicsShader

class CRHIGraphicsShader : public FRHIShader
{
protected:

    explicit CRHIGraphicsShader(EShaderStage InShaderStage)
        : FRHIShader(InShaderStage)
    { }

    ~CRHIGraphicsShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIVertexShader

class FRHIVertexShader : public CRHIGraphicsShader
{
protected:

    FRHIVertexShader()
        : CRHIGraphicsShader(EShaderStage::Vertex)
    { }

    ~FRHIVertexShader() = default;
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
// FRHIPixelShader

class FRHIPixelShader : public CRHIGraphicsShader
{
protected:

    FRHIPixelShader()
        : CRHIGraphicsShader(EShaderStage::Pixel)
    { }

    ~FRHIPixelShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingShader

class FRHIRayTracingShader : public FRHIShader
{
protected:

    explicit FRHIRayTracingShader(EShaderStage InShaderStage)
        : FRHIShader(InShaderStage)
    { }

    ~FRHIRayTracingShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayGenShader

class FRHIRayGenShader : public FRHIRayTracingShader
{
protected:

    FRHIRayGenShader()
        : FRHIRayTracingShader(EShaderStage::RayGen)
    { }

    ~FRHIRayGenShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayAnyHitShader

class FRHIRayAnyHitShader : public FRHIRayTracingShader
{
protected:

    FRHIRayAnyHitShader()
        : FRHIRayTracingShader(EShaderStage::RayAnyHit)
    { }

    ~FRHIRayAnyHitShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayClosestHitShader

class FRHIRayClosestHitShader : public FRHIRayTracingShader
{
protected:

    FRHIRayClosestHitShader()
        : FRHIRayTracingShader(EShaderStage::RayClosestHit)
    { }

    ~FRHIRayClosestHitShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayMissShader

class FRHIRayMissShader : public FRHIRayTracingShader
{
protected:

    FRHIRayMissShader()
        : FRHIRayTracingShader(EShaderStage::RayMiss)
    { }

    ~FRHIRayMissShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayIntersectionShader

class CRHIRayIntersectionShader : public FRHIRayTracingShader
{
protected:

    CRHIRayIntersectionShader()
        : FRHIRayTracingShader(EShaderStage::RayIntersection)
    { }

    ~CRHIRayIntersectionShader() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayCallableShader

class CRHIRayCallableShader : public FRHIRayTracingShader
{
protected:

    CRHIRayCallableShader()
        : FRHIRayTracingShader(EShaderStage::RayCallable)
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
