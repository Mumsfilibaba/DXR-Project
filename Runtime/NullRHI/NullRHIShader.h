#pragma once
#include "RHI/RHIShader.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

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

struct FNullRHIComputeShaderBase : public FRHIComputeShader
{
    FNullRHIComputeShaderBase()
        : FRHIComputeShader()
    {
    }

    virtual FIntVector3 GetThreadGroupXYZ() const override final { return FIntVector3(1, 1, 1); }
};

template<typename BaseShaderType>
struct TNullRHIShader final : public BaseShaderType
{
    TNullRHIShader()
        : BaseShaderType()
    {
    }

    virtual void* GetRHIBaseShader() override final { return this; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
