#pragma once
#include "MetalViews.h"

#include "RHI/RHIRayTracing.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayTracingGeometry

class CMetalRayTracingGeometry : public FRHIRayTracingGeometry
{
public:

    CMetalRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
        : FRHIRayTracingGeometry(Initializer)
    { }

    ~CMetalRayTracingGeometry() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIRayTracingGeometry Interface

    virtual void* GetRHIBaseBVHBuffer() { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() { return reinterpret_cast<void*>(this); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayTracingScene

class CMetalRayTracingScene : public FRHIRayTracingScene
{
public:
    CMetalRayTracingScene(CMetalDeviceContext* InDeviceContext, const FRHIRayTracingSceneInitializer& Initializer)
        : FRHIRayTracingScene(Initializer)
        , View(dbg_new CMetalShaderResourceView(InDeviceContext, this))
    { }

    ~CMetalRayTracingScene() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIRayTracingScene Interface

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final{ return FRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer() override final { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(this); }

private:
    TSharedRef<CMetalShaderResourceView> View;
};

#pragma clang diagnostic pop
