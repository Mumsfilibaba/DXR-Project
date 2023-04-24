#pragma once
#include "RHIResources.h"

#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"

typedef TSharedRef<class FRHIShader>                FRHIShaderRef;

typedef TSharedRef<class FRHIVertexShader>          FRHIVertexShaderRef;
typedef TSharedRef<class FRHIHullShader>            FRHIHullShaderRef;
typedef TSharedRef<class FRHIDomainShader>          FRHIDomainShaderRef;
typedef TSharedRef<class FRHIGeometryShader>        FRHIGeometryShaderRef;
typedef TSharedRef<class FRHIMeshShader>            FRHIMeshShaderRef;
typedef TSharedRef<class FRHIAmplificationShader>   FRHIAmplificationShaderRef;
typedef TSharedRef<class FRHIPixelShader>           FRHIPixelShaderRef;

typedef TSharedRef<class FRHIComputeShader>         FRHIComputeShaderRef;

typedef TSharedRef<class FRHIRayTracingShader>      FRHIRayTracingShaderRef;
typedef TSharedRef<class FRHIRayGenShader>          FRHIRayGenShaderRef;
typedef TSharedRef<class FRHIRayMissShader>         FRHIRayMissShaderRef;
typedef TSharedRef<class FRHIRayClosestHitShader>   FRHIRayClosestHitShaderRef;
typedef TSharedRef<class FRHIRayAnyHitShader>       FRHIRayAnyHitShaderRef;
typedef TSharedRef<class FRHIRayIntersectionShader> FRHIRayIntersectionShaderRef;
typedef TSharedRef<class FRHIRayCallableShader>     FRHIRayCallableShaderRef;


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

constexpr const CHAR* ToString(EShaderStage ShaderStage)
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


class FRHIShader 
    : public FRHIResource
{
protected:
    explicit FRHIShader(EShaderStage InShaderStage)
        : ShaderStage(InShaderStage)
    { }

public:
    virtual void* GetRHIBaseResource() { return nullptr; }
    virtual void* GetRHIBaseShader()   { return nullptr; }
    
    EShaderStage GetShaderStage() const { return ShaderStage; }

private:
    EShaderStage ShaderStage;
};

class FRHIComputeShader
    : public FRHIShader
{
protected:
    FRHIComputeShader()
        : FRHIShader(EShaderStage::Compute)
    { }

public:

    /** @return - Returns a vector with the number of threads in each dimension */
    virtual FIntVector3 GetThreadGroupXYZ() const = 0;
};

class FRHIGraphicsShader 
    : public FRHIShader
{
protected:
    explicit FRHIGraphicsShader(EShaderStage InShaderStage)
        : FRHIShader(InShaderStage)
    { }
};

class FRHIVertexShader 
    : public FRHIGraphicsShader
{
protected:
    FRHIVertexShader()
        : FRHIGraphicsShader(EShaderStage::Vertex)
    { }
};

class FRHIHullShader 
    : public FRHIGraphicsShader
{
protected:
    FRHIHullShader()
        : FRHIGraphicsShader(EShaderStage::Hull)
    { }
};

class FRHIDomainShader 
    : public FRHIGraphicsShader
{
protected:
    FRHIDomainShader()
        : FRHIGraphicsShader(EShaderStage::Domain)
    { }
};

class FRHIGeometryShader 
    : public FRHIGraphicsShader
{
protected:
    FRHIGeometryShader()
        : FRHIGraphicsShader(EShaderStage::Geometry)
    { }
};

class FRHIMeshShader 
    : public FRHIGraphicsShader
{
protected:
    FRHIMeshShader()
        : FRHIGraphicsShader(EShaderStage::Mesh)
    { }
};

class FRHIAmplificationShader 
    : public FRHIGraphicsShader
{
protected:
    FRHIAmplificationShader()
        : FRHIGraphicsShader(EShaderStage::Amplification)
    { }
};

class FRHIPixelShader 
    : public FRHIGraphicsShader
{
protected:
    FRHIPixelShader()
        : FRHIGraphicsShader(EShaderStage::Pixel)
    { }
};

class FRHIRayTracingShader 
    : public FRHIShader
{
protected:
    explicit FRHIRayTracingShader(EShaderStage InShaderStage)
        : FRHIShader(InShaderStage)
    { }
};

class FRHIRayGenShader 
    : public FRHIRayTracingShader
{
protected:
    FRHIRayGenShader()
        : FRHIRayTracingShader(EShaderStage::RayGen)
    { }
};

class FRHIRayAnyHitShader 
    : public FRHIRayTracingShader
{
protected:
    FRHIRayAnyHitShader()
        : FRHIRayTracingShader(EShaderStage::RayAnyHit)
    { }
};

class FRHIRayClosestHitShader
    : public FRHIRayTracingShader
{
protected:
    FRHIRayClosestHitShader()
        : FRHIRayTracingShader(EShaderStage::RayClosestHit)
    { }
};

class FRHIRayMissShader 
    : public FRHIRayTracingShader
{
protected:
    FRHIRayMissShader()
        : FRHIRayTracingShader(EShaderStage::RayMiss)
    { }
};

class FRHIRayIntersectionShader 
    : public FRHIRayTracingShader
{
protected:
    FRHIRayIntersectionShader()
        : FRHIRayTracingShader(EShaderStage::RayIntersection)
    { }
};

class FRHIRayCallableShader
    : public FRHIRayTracingShader
{
protected:
    FRHIRayCallableShader()
        : FRHIRayTracingShader(EShaderStage::RayCallable)
    { }
};


constexpr bool ShaderStageIsGraphics(EShaderStage ShaderStage)
{
    return ((ShaderStage >= EShaderStage::Vertex) && (ShaderStage < EShaderStage::Compute)) ? true : false;
}

/** @brief - Determine if the Compute Pipeline is used (DXR uses the compute pipeline for RootSignatures) */
constexpr bool ShaderStageIsCompute(EShaderStage ShaderStage)
{
    return (ShaderStage >= EShaderStage::Compute) ? true : false;
}

constexpr bool ShaderStageIsRayTracing(EShaderStage ShaderStage)
{
    return (ShaderStage >= EShaderStage::RayGen) ? true : false;
}
