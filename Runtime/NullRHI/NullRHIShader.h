#pragma once
#include "RHI/RHIShader.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

template<typename T>
struct TNullRHIShader;

typedef TNullRHIShader<class FRHIVertexShader>          FNullVertexShaderRHI;
typedef TNullRHIShader<class FRHIHullShader>            FNullHullShaderRHI;
typedef TNullRHIShader<class FRHIDomainShader>          FNullDomainShaderRHI;
typedef TNullRHIShader<class FRHIGeometryShader>        FNullGeometryShaderRHI;
typedef TNullRHIShader<class FRHIAmplificationShader>   FNullAmplificationShaderRHI;
typedef TNullRHIShader<class FRHIMeshShader>            FNullMeshShaderRHI;
typedef TNullRHIShader<class FRHIPixelShader>           FNullPixelShaderRHI;
typedef TNullRHIShader<class FRHIComputeShader>         FNullComputeShaderRHI;
typedef TNullRHIShader<class FRHIRayTracingShader>      FNullRayTracingShaderRHI;
typedef TNullRHIShader<class FRHIRayGenShader>          FNullRayGenShaderRHI;
typedef TNullRHIShader<class FRHIRayMissShader>         FNullRayMissShaderRHI;
typedef TNullRHIShader<class FRHIRayClosestHitShader>   FNullRayClosestHitShaderRHI;
typedef TNullRHIShader<class FRHIRayAnyHitShader>       FNullRayAnyHitShaderRHI;
typedef TNullRHIShader<class FRHIRayIntersectionShader> FNullRayIntersectionShaderRHI;
typedef TNullRHIShader<class FRHIRayCallableShader>     FNullRayCallableShaderRHI;

template<typename BaseShaderType>
struct TNullRHIShader final : public BaseShaderType
{
    TNullRHIShader()
        : BaseShaderType()
    {
    }

    virtual void* GetRHINativeHandle() override final
    {
        return nullptr;
    }

    virtual void* GetRHIBaseInterface() override final
    {
        return this;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
