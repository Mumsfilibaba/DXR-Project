#pragma once
#include "MetalViews.h"

#include "RHI/RHIRayTracing.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

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

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
