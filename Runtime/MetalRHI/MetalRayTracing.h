#pragma once
#include "MetalViews.h"

#include "RHI/RHIRayTracing.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalRayTracingGeometry

class FMetalRayTracingGeometry 
    : public FRHIRayTracingGeometry
{
public:
    FMetalRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
        : FRHIRayTracingGeometry(Initializer)
    { }

    ~FMetalRayTracingGeometry() = default;

    virtual void* GetRHIBaseBVHBuffer()             { return nullptr; }
    virtual void* GetRHIBaseAccelerationStructure() { return reinterpret_cast<void*>(this); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalRayTracingScene

class FMetalRayTracingScene 
    : public FRHIRayTracingScene
{
public:
    FMetalRayTracingScene(FMetalDeviceContext* InDeviceContext, const FRHIRayTracingSceneInitializer& Initializer)
        : FRHIRayTracingScene(Initializer)
        , View(dbg_new FMetalShaderResourceView(InDeviceContext, this))
    { }

    ~FMetalRayTracingScene() = default;

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final{ return FRHIDescriptorHandle(); }
 
    virtual void* GetRHIBaseBVHBuffer()             override final { return nullptr; }
    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(this); }

private:
    TSharedRef<FMetalShaderResourceView> View;
};

#pragma clang diagnostic pop
