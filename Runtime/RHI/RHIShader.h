#pragma once
#include "Core/Math/IntVector3.h"
#include "Core/Containers/String.h"
#include "Core/Containers/ArrayView.h"
#include "RHI/RHIResources.h"
#include "ShaderCore/ShaderTypes.h"
#include "ShaderCore/ShaderReflection.h"

typedef TSharedRef<class FRHIShader>                FRHIShaderRef;
typedef TSharedRef<class FRHIGraphicsShader>        FRHIGraphicsShaderRef;
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

class FRHIShader : public FRHIResource
{
protected:
    explicit FRHIShader(EShaderStage InShaderStage)
        : FRHIResource(ERHIResourceType::Shader)
        , ShaderStage(InShaderStage)
    {
    }

    virtual ~FRHIShader() = default;

public:

    /** @return D3D12: D3D12_SHADER_BYTECODE*. Vulkan: TArray<uint32>* (SPIR-V). Metal: id<MTLFunction>. Null: nullptr. */
    virtual void* GetRHINativeHandle()  = 0;
    virtual void* GetRHIBaseInterface() = 0;

    EShaderStage GetShaderStage() const
    { 
        return ShaderStage;
    }

private:
    EShaderStage ShaderStage;
};

class FRHIComputeShader : public FRHIShader
{
protected:
    FRHIComputeShader()
        : FRHIShader(EShaderStage::Compute)
    {
    }

    virtual ~FRHIComputeShader() = default;
};

class FRHIGraphicsShader : public FRHIShader
{
protected:
    explicit FRHIGraphicsShader(EShaderStage InShaderStage)
        : FRHIShader(InShaderStage)
    {
    }

    virtual ~FRHIGraphicsShader() = default;
};

class FRHIVertexShader : public FRHIGraphicsShader
{
protected:
    FRHIVertexShader()
        : FRHIGraphicsShader(EShaderStage::Vertex)
        , VertexInputs()
    {
    }

    virtual ~FRHIVertexShader() = default;

public:

    /** Sorted by Location, empty for shaders that only read system values */
    const TArray<FShaderVertexInput>& GetVertexInputs() const
    {
        return VertexInputs;
    }

    void SetVertexInputs(TArrayView<const FShaderVertexInput> InVertexInputs)
    {
        VertexInputs = TArray<FShaderVertexInput>(InVertexInputs.Data(), InVertexInputs.Size());
    }

private:
    TArray<FShaderVertexInput> VertexInputs;
};

class FRHIHullShader : public FRHIGraphicsShader
{
protected:
    FRHIHullShader()
        : FRHIGraphicsShader(EShaderStage::Hull)
    {
    }

    virtual ~FRHIHullShader() = default;
};

class FRHIDomainShader : public FRHIGraphicsShader
{
protected:
    FRHIDomainShader()
        : FRHIGraphicsShader(EShaderStage::Domain)
    {
    }

    virtual ~FRHIDomainShader() = default;
};

class FRHIGeometryShader : public FRHIGraphicsShader
{
protected:
    FRHIGeometryShader()
        : FRHIGraphicsShader(EShaderStage::Geometry)
    {
    }

    virtual ~FRHIGeometryShader() = default;
};

class FRHIMeshShader : public FRHIGraphicsShader
{
protected:
    FRHIMeshShader()
        : FRHIGraphicsShader(EShaderStage::Mesh)
    {
    }

    virtual ~FRHIMeshShader() = default;
};

class FRHIAmplificationShader : public FRHIGraphicsShader
{
protected:
    FRHIAmplificationShader()
        : FRHIGraphicsShader(EShaderStage::Amplification)
    {
    }

    virtual ~FRHIAmplificationShader() = default;
};

class FRHIPixelShader : public FRHIGraphicsShader
{
protected:
    FRHIPixelShader()
        : FRHIGraphicsShader(EShaderStage::Pixel)
    {
    }

    virtual ~FRHIPixelShader() = default;
};

class FRHIRayTracingShader : public FRHIShader
{
protected:
    explicit FRHIRayTracingShader(EShaderStage InShaderStage)
        : FRHIShader(InShaderStage)
    {
    }

    virtual ~FRHIRayTracingShader() = default;
};

class FRHIRayGenShader : public FRHIRayTracingShader
{
protected:
    FRHIRayGenShader()
        : FRHIRayTracingShader(EShaderStage::RayGen)
    {
    }

    virtual ~FRHIRayGenShader() = default;
};

class FRHIRayAnyHitShader : public FRHIRayTracingShader
{
protected:
    FRHIRayAnyHitShader()
        : FRHIRayTracingShader(EShaderStage::RayAnyHit)
    {
    }

    virtual ~FRHIRayAnyHitShader() = default;
};

class FRHIRayClosestHitShader : public FRHIRayTracingShader
{
protected:
    FRHIRayClosestHitShader()
        : FRHIRayTracingShader(EShaderStage::RayClosestHit)
    {
    }

    virtual ~FRHIRayClosestHitShader() = default;
};

class FRHIRayMissShader : public FRHIRayTracingShader
{
protected:
    FRHIRayMissShader()
        : FRHIRayTracingShader(EShaderStage::RayMiss)
    {
    }

    virtual ~FRHIRayMissShader() = default;
};

class FRHIRayIntersectionShader : public FRHIRayTracingShader
{
protected:
    FRHIRayIntersectionShader()
        : FRHIRayTracingShader(EShaderStage::RayIntersection)
    {
    }

    virtual ~FRHIRayIntersectionShader() = default;
};

class FRHIRayCallableShader : public FRHIRayTracingShader
{
protected:
    FRHIRayCallableShader()
        : FRHIRayTracingShader(EShaderStage::RayCallable)
    {
    }

    virtual ~FRHIRayCallableShader() = default;
};
