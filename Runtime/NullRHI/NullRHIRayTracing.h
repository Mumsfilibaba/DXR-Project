#pragma once
#include "NullRHIViews.h"

#include "RHI/RHIRayTracing.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIRayTracingGeometry

struct FNullRHIRayTracingGeometry 
    : public FRHIRayTracingGeometry
{
    explicit FNullRHIRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
        : FRHIRayTracingGeometry(Initializer)
    { }

    ~FNullRHIRayTracingGeometry() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIRayTracingGeometry Interface

    virtual void* GetRHIBaseBVHBuffer()             { return nullptr; }
    virtual void* GetRHIBaseAccelerationStructure() { return reinterpret_cast<void*>(this); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIRayTracingScene

class FNullRHIRayTracingScene 
    : public FRHIRayTracingScene
{
public:
    explicit FNullRHIRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
        : FRHIRayTracingScene(Initializer)
        , View(dbg_new FNullRHIShaderResourceView(this))
    { }

    ~FNullRHIRayTracingScene() = default;

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIRayTracingScene Interface

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final { return FRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer()             override final { return nullptr; }
    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(this); }

private:
    TSharedRef<FNullRHIShaderResourceView> View;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
