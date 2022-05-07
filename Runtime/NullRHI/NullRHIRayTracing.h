#pragma once
#include "NullRHIViews.h"

#include "RHI/RHIRayTracing.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingGeometry

class CNullRHIRayTracingGeometry : public CRHIRayTracingGeometry
{
public:

    CNullRHIRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
        : CRHIRayTracingGeometry(Initializer)
    { }

    ~CNullRHIRayTracingGeometry() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIRayTracingGeometry Interface

    virtual void* GetRHIBaseBVHBuffer() { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() { return reinterpret_cast<void*>(this); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIRayTracingScene

class CNullRHIRayTracingScene : public CRHIRayTracingScene
{
public:
    CNullRHIRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
        : CRHIRayTracingScene(Initializer)
        , View(dbg_new CNullRHIShaderResourceView(this))
    { }

    ~CNullRHIRayTracingScene() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIRayTracingScene Interface

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }

    virtual CRHIDescriptorHandle GetBindlessHandle() const override final{ return CRHIDescriptorHandle(); }

    virtual void* GetRHIBaseBVHBuffer() override final { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(this); }

private:
    TSharedRef<CNullRHIShaderResourceView> View;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
