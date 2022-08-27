#pragma once
#include "RHI/RHIResources.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

template<typename T>
struct TNullRHIShader;

typedef TNullRHIShader<class FRHIVertexShader>           FNullRHIVertexShader;
typedef TNullRHIShader<class FRHIHullShader>             FNullRHIHullShader;
typedef TNullRHIShader<class FRHIDomainShader>           FNullRHIDomainShader;
typedef TNullRHIShader<class FRHIGeometryShader>         FNullRHIGeometryShader;
typedef TNullRHIShader<class FRHIAmplificationShader>    FNullRHIAmplificationShader;
typedef TNullRHIShader<class FRHIMeshShader>             FNullRHIMeshShader;
typedef TNullRHIShader<class FRHIPixelShader>            FNullRHIPixelShader;

typedef TNullRHIShader<struct FNullRHIComputeShaderBase> FNullRHIComputeShader;

typedef TNullRHIShader<class FRHIRayTracingShader>       FNullRHIRayTracingShader;
typedef TNullRHIShader<class FRHIRayGenShader>           FNullRHIRayGenShader;
typedef TNullRHIShader<class FRHIRayMissShader>          FNullRHIRayMissShader;
typedef TNullRHIShader<class FRHIRayClosestHitShader>    FNullRHIRayClosestHitShader;
typedef TNullRHIShader<class FRHIRayAnyHitShader>        FNullRHIRayAnyHitShader;
typedef TNullRHIShader<class FRHIRayIntersectionShader>  FNullRHIRayIntersectionShader;
typedef TNullRHIShader<class FRHIRayCallableShader>      FNullRHIRayCallableShader;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIComputeShader

struct FNullRHIComputeShaderBase 
    : public FRHIComputeShader
{
    FNullRHIComputeShaderBase()
        : FRHIComputeShader()
    { }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIComputeShader Interface

    virtual FIntVector3 GetThreadGroupXYZ() const override final { return FIntVector3(1, 1, 1); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHIShader

template<typename BaseShaderType>
struct TNullRHIShader final 
    : public BaseShaderType
{
    TNullRHIShader()
        : BaseShaderType()
    { }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIShader Interface

    virtual void* GetRHIBaseShader() override final { return this; }
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
