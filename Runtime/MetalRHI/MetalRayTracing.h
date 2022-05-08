#pragma once
#include "MetalViews.h"

#include "RHI/RHIRayTracing.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayTracingGeometry

class CMetalRayTracingGeometry : public CRHIRayTracingGeometry
{
public:

    CMetalRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
        : CRHIRayTracingGeometry(Initializer)
    { }

    ~CMetalRayTracingGeometry() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIRayTracingGeometry Interface

    virtual void* GetRHIBaseBVHBuffer() { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() { return reinterpret_cast<void*>(this); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayTracingScene

class CMetalRayTracingScene : public CRHIRayTracingScene
{
public:
    CMetalRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
        : CRHIRayTracingScene(Initializer)
        , View(dbg_new CMetalShaderResourceView(this))
    { }

    ~CMetalRayTracingScene() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIRayTracingScene Interface

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }

    virtual CRHIDescriptorHandle GetBindlessHandle() const override final{ return CRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer() override final { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(this); }

private:
    TSharedRef<CMetalShaderResourceView> View;
};

#pragma clang diagnostic pop
